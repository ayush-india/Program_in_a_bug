#include <SFML/Graphics.hpp>
#include "engine/engine.hpp"
#include "engine/render/viewport_handler.hpp"
#include "engine/render/renderer.hpp"
#include "engine/common/event_manager.hpp"
#include "engine/common/number_generator.hpp"
#include "engine/common/sequence.hpp"
#include "engine/common/transition.hpp"


struct SimulationParameters
{
    float response_coef;
    float vel_coef;
    float force;
    float drag_coef;
    float zoom;
};


int main()
{
    // Create context
    const int32_t window_width = 800;
    const int32_t window_height = 800;
    sf::ContextSettings settings;
    settings.antialiasingLevel = 2;
    sf::RenderWindow window(sf::VideoMode(window_width, window_height), "NovaBug", sf::Style::Default, settings);
    int32_t base_fps = 45;
    window.setFramerateLimit(base_fps);
    window.setMouseCursorVisible(true);
    window.setKeyRepeatEnabled(false);

    bool slow_mo = false;
    bool pause = false;
    
    Engine engine(500, 500);
    ViewportHandler vp_handler(toVector2f(sf::Vector2i(window_width, window_height)));
    vp_handler.setFocus({ 250.0f, 250.0f });
    vp_handler.setZoom(2.5f);
    // Events init
    sfev::EventManager ev_manager(window, true);
    ev_manager.addEventCallback(sf::Event::Closed, [&](sfev::CstEv) { window.close(); });
    ev_manager.addKeyPressedCallback(sf::Keyboard::Escape, [&](sfev::CstEv) { window.close(); });
    ev_manager.addKeyPressedCallback(sf::Keyboard::S, [&](sfev::CstEv) { slow_mo = !slow_mo; window.setFramerateLimit(base_fps * slow_mo); });
    ev_manager.addMousePressedCallback(sf::Mouse::Left, [&](sfev::CstEv) {
        vp_handler.click(ev_manager.getFloatMousePosition());
    });
    ev_manager.addMouseReleasedCallback(sf::Mouse::Left, [&](sfev::CstEv) {
        vp_handler.unclick();
    });
    ev_manager.addEventCallback(sf::Event::MouseMoved, [&](sfev::CstEv) {
        vp_handler.setMousePosition(ev_manager.getFloatMousePosition());
    });
    ev_manager.addEventCallback(sf::Event::Closed, [&](sfev::CstEv) {
        ev_manager.getWindow().close();
    });
    ev_manager.addKeyPressedCallback(sf::Keyboard::Escape, [&](sfev::CstEv) {
        ev_manager.getWindow().close();
    });
    ev_manager.addEventCallback(sf::Event::MouseWheelScrolled, [&](sfev::CstEv e) {
        vp_handler.wheelZoom(e.mouseWheelScroll.delta);
    });
    ev_manager.addKeyPressedCallback(sf::Keyboard::Space, [&](sfev::CstEv) {
        pause = !pause;
    });
    // Simulation init
    const float dt = 0.016f;
    const uint32_t objects_count = 16000;
    for (uint32_t i(objects_count); i--;) {
        PhysicObject o;
        o.setPosition({ RNGf::getUnder(engine.solver.world_size.x), RNGf::getUnder(engine.solver.world_size.y) });
        engine.addEntity(o);
    }
    engine.solver.gravity = sf::Vector2f(0.0f, 0.0f);
    // Rendering init
    sf::RenderTexture render_texture;
    render_texture.create(window_width, window_height);
    render_texture.clear(sf::Color::Black);
    sf::Sprite render_sprite(render_texture.getTexture());
    Renderer renderer(render_texture, engine.solver.objects);
    // Simulation parameters
    Sequence<SimulationParameters> parameters_sequence;
    parameters_sequence.add({0.6f, 0.0f, 47.0f, 0.01f, 1.7f},     30.0f);
    parameters_sequence.add({1.0f, 0.02f, 42.0f, 0.01f, 3.0f},    30.0f);
    parameters_sequence.add({0.55f, 0.01f, 70.0f, 0.0003f, 4.5f}, 25.0f);
    parameters_sequence.add({0.7f, 0.001f, 52.0f, 0.01f, 2.5f},   20.0f);
    parameters_sequence.add({1.1f, 0.1f, 45.0f, 0.01f, 3.5f},     30.0f);
    parameters_sequence.add({0.8f, 0.02f, 80.0f, 0.02f, 2.8f},    30.0f);
    parameters_sequence.add({0.45f, 0.03f, 100.0f, 0.0f, 2.5f},   20.0f);
    parameters_sequence.add({0.8f, 0.01f, 40.0f, 0.001f, 1.5f},   30.0f);
    parameters_sequence.add({0.45f, 0.03f, 100.0f, 0.0f, 2.5f},   20.0f);
    parameters_sequence.add({1.01f, 0.05f, 90.0f, 0.01f, 3.8f},   30.0f);
    trn::Transition<float> current_zoom = parameters_sequence.getCurrent().zoom;

    sf::RectangleShape fade({ window_width, window_height });
    fade.setFillColor({ 0, 0, 0, 10 });

    while (window.isOpen()) {
        ev_manager.processEvents();
        vp_handler.setZoom(current_zoom);
        // Update simulation parameters
        const SimulationParameters& current_parameters = parameters_sequence.getCurrent();
        engine.solver.response_coef = current_parameters.response_coef;
        engine.solver.vel_coef = current_parameters.vel_coef;
        if (current_zoom.getTarget() != current_parameters.zoom) {
            current_zoom = current_parameters.zoom;
        }
        // Physics update
        if (!pause) {
            parameters_sequence.update(dt);
            const sf::Vector2f center = engine.solver.world_size * 0.5f;
            for (PhysicObject& obj : engine.solver.objects) {
                const sf::Vector2f to_center = center - obj.position;
                const sf::Vector2f to_center_v = to_center / getLength(to_center);
                obj.accelerate(to_center_v * current_parameters.force);
            }
            engine.update(dt);
            // Air friction
            for (PhysicObject& obj : engine.solver.objects) {
                obj.slowdown(current_parameters.drag_coef / (1.0f + obj.pressure));
            }
        }
        // Render
        window.clear();
        render_texture.draw(fade);
        renderer.render(vp_handler.getRenderState());
        // Draw the frame
        render_texture.display();
        window.draw(render_sprite);
        window.display();
    }
    
	return 0;
}
