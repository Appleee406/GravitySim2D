#include <SFML/Graphics.hpp>
#include <cmath>
#include <vector>
#include <iostream>

constexpr float G = 6.6743e-11;
constexpr float METRS_PER_PIXEL = 2900000e2f;
constexpr float DT = 70000.f;

constexpr float SCREEN_W = 600.f;
constexpr float SCREEN_H = 600.f;

class Body : public sf::Drawable {
public:
    sf::Color m_color;
    sf::Vector2f m_pos;
    sf::Vector2f m_vel;           // in m/s
    sf::Vector2f m_acceleration;  // in m/s^2
    float m_radius;  // in meters
    float m_mass;    // in kilogramms

    Body(sf::Vector2f pos, sf::Vector2f vel, float radius, float mass, sf::Color color=sf::Color::Cyan)
        : m_color(color), m_pos(pos), m_vel(vel), m_radius(radius), m_mass(mass) {}

    void draw(sf::RenderTarget& target, sf::RenderStates states) const override {
        float r_pix = m_radius / METRS_PER_PIXEL;
        if(r_pix < 4.f) r_pix = 4.f;
        if(m_mass > 1e29f && r_pix < 12.f) r_pix = 12.f;

        sf::CircleShape shape(r_pix);
        shape.setFillColor(m_color);
        shape.setOrigin({r_pix, r_pix});
        shape.setPosition({
            m_pos.x / METRS_PER_PIXEL,
            m_pos.y / METRS_PER_PIXEL
        });

        target.draw(shape, states);
    }
};

void updatePhysics(std::vector<Body>& bodies, float dt) {
    static std::vector<sf::Vector2f> a_old;
    if(a_old.size() != bodies.size()) a_old.resize(bodies.size());

    for(size_t i = 0; i < bodies.size(); ++i) {
        bodies[i].m_pos += bodies[i].m_vel * dt + 0.5f *bodies[i].m_acceleration * std::powf(dt, 2);
        a_old[i] = bodies[i].m_acceleration;
        bodies[i].m_acceleration = sf::Vector2f(0.f, 0.f);
    }

    const float MIN_DIST = 100000.f;  // 100 km

    for(size_t i = 0; i < bodies.size(); ++i) {
        for(size_t j = i + 1; j < bodies.size(); ++j) {
            sf::Vector2f delta = bodies[j].m_pos - bodies[i].m_pos; // (dx, dy);

            float dist_sqr = delta.x * delta.x + delta.y * delta.y;
            float dist = std::sqrtf(dist_sqr);

            if (dist < MIN_DIST) {
                dist = MIN_DIST;
                dist_sqr = MIN_DIST * MIN_DIST;
            }

            float force = G / (dist_sqr * dist);

            bodies[i].m_acceleration += delta * force * bodies[j].m_mass;
            bodies[j].m_acceleration -= delta * force * bodies[i].m_mass;
        }
    }

    for(size_t i = 0; i < bodies.size(); ++i) {
        bodies[i].m_vel += 0.5f * (a_old[i] + bodies[i].m_acceleration) * dt;
    }
}

int main() {
    sf::RenderWindow window(
        sf::VideoMode({static_cast<unsigned int>(SCREEN_W), static_cast<unsigned int>(SCREEN_H)}),
        "2D Gravity Simulation"
    );
    window.setFramerateLimit(120);

    sf::View cam({0.f, 0.f}, {SCREEN_W, SCREEN_H});
    constexpr float CAM_SPEED = 10.f;
    constexpr float ZOOM_SCALE = 0.1;
    constexpr float MIN_W = 150.f;
    constexpr float MAX_W = 20000.f;

    constexpr float AU = 149597870700.f;  // astronomical unit
    constexpr float EM = 5.9722e24f;      // earth mass
    constexpr float ER = 6378.1f;         // earth radius

    Body sun({0.f, 0.f}, {0.f, 0.f}, 109.2032f * ER, 332946.f * EM, sf::Color::Yellow);
    Body mercury({0.387098f * AU, 0.f}, {0.f, 47362.f}, 0.38251f * ER, 5.5274e-2f * EM, sf::Color(77, 77, 77));
    Body venus({0.723332f * AU, 0.f}, {0.f, 35020.f}, 0.94884f * ER, 0.81499f * EM, sf::Color(186, 147, 28));
    Body earth({AU, 0.f}, {0.f, 29780.f}, ER, EM);
    Body mars({1.523679f * AU, 0.f}, {0.f, 24077.f}, 0.53248f * ER, 0.10745f * EM, sf::Color::Red);
    Body jupiter({5.204407f * AU, 0.f}, {0.f, 13070.f}, 11.2092f * ER, 3.17828e2f * EM, sf::Color(186, 96, 28));
    Body saturn({9.582565f * AU, 0.f}, {0.f, 9690.f}, 9.44917f * ER, 9.516090f * EM, sf::Color(179, 149, 109));
    Body uranus({19.218446f * AU, 0.f}, {0.f, 6800.f}, 4.00732f * ER, 1.453570f * EM, sf::Color(163, 240, 226));
    Body neptune({ 30.069923f * AU, 0.f}, {0.f, 5430.f}, 3.88266f * ER, 1.714710f * EM, sf::Color::Blue);

    std::vector<Body> bodies = {sun, mercury, venus, earth, mars, jupiter, saturn, uranus, neptune};

    sf::Texture background_texture;
    bool bg_load = true;
    if(!background_texture.loadFromFile("../assets/space.png")) {
        std::cerr << "assets/space.png not found!" << '\n';
        bg_load = false;
    }
    background_texture.setRepeated(true);
    sf::Sprite background(background_texture);

    sf::View bg_view({SCREEN_W / 2.f, SCREEN_H / 2.f}, {SCREEN_W, SCREEN_H});

    bool is_paused = true;
    while (window.isOpen()) {
        while (const std::optional event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) window.close();

            if(const auto* key = event->getIf<sf::Event::KeyPressed>()) {
                if(key->code == sf::Keyboard::Key::Space) is_paused = !is_paused;
            }

            if(const auto* wheel_e = event->getIf<sf::Event::MouseWheelScrolled>()) {
                if(wheel_e->wheel == sf::Mouse::Wheel::Vertical) {
                    float zoom = (wheel_e->delta > 0) ? (1.f / (1.f + ZOOM_SCALE)) : (1.f + ZOOM_SCALE);
                    float predicted_width = cam.getSize().x * zoom;

                    if(predicted_width >= MIN_W && predicted_width <= MAX_W) cam.zoom(zoom);
                    else {
                        if (predicted_width < MIN_W) {
                            float exact_zoom = MIN_W / cam.getSize().x;
                            cam.zoom(exact_zoom);
                        } else if (predicted_width > MAX_W) {
                            float exact_zoom = MAX_W / cam.getSize().x;
                            cam.zoom(exact_zoom);
                        }
                    }
                }
            }

            if(const auto* resized = event->getIf<sf::Event::Resized>()) {
                sf::Vector2f new_size = static_cast<sf::Vector2f>(resized->size);

                background.setTextureRect(sf::IntRect({0, 0}, static_cast<sf::Vector2i>(resized->size)));
                cam.setSize(new_size);

                bg_view.setSize(new_size);
                bg_view.setCenter(new_size / 2.f);
            }
        }

        if(sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S)) cam.move({0.f, CAM_SPEED});
        if(sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W)) cam.move({0.f, -CAM_SPEED});
        if(sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D)) cam.move({CAM_SPEED, 0.f});
        if(sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A)) cam.move({-CAM_SPEED, 0.f});

        if(!is_paused) updatePhysics(bodies, DT);

        window.clear();
        
        if(bg_load) {
            window.setView(bg_view);
            window.draw(background);
        }
        window.setView(cam);
        
        for(const auto& body: bodies) {
            sf::Vector2f screen_pos = {
                body.m_pos.x / METRS_PER_PIXEL,
                body.m_pos.y / METRS_PER_PIXEL
            };

            float cam_left = cam.getCenter().x - (cam.getSize().x / 2.f);
            float cam_right = cam.getCenter().x + (cam.getSize().x / 2.f);
            float cam_top = cam.getCenter().y - (cam.getSize().y / 2.f);
            float cam_bottom = cam.getCenter().y + (cam.getSize().y / 2.f);

            float radius_pixels = body.m_radius / METRS_PER_PIXEL;

            if (screen_pos.x + radius_pixels > cam_left   &&
                screen_pos.x - radius_pixels < cam_right  &&
                screen_pos.y + radius_pixels > cam_top    &&
                screen_pos.y - radius_pixels < cam_bottom
            ) window.draw(body);
        }

        window.display();
    }

    return 0;
}