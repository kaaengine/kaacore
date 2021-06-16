#include <algorithm>
#include <memory>
#include <vector>

#include <glm/glm.hpp>
#include <spdlog/fmt/fmt.h>

#include "kaacore/clock.h"
#include "kaacore/engine.h"
#include "kaacore/input.h"
#include "kaacore/log.h"
#include "kaacore/nodes.h"
#include "kaacore/scenes.h"

struct InputDemoScene : kaacore::Scene {
    kaacore::NodePtr timer_txt;
    kaacore::NodePtr box;
    kaacore::NodePtr cursor;
    kaacore::TimePoint start_time;
    bool clicked = false;

    InputDemoScene()
    {
        this->start_time = kaacore::Clock::now();

        auto _timer_txt = kaacore::make_node(kaacore::NodeType::text);
        _timer_txt->position({-48., -45.});
        _timer_txt->origin_alignment(kaacore::Alignment::top_left);
        this->timer_txt = this->root_node.add_child(_timer_txt);

        auto _box = kaacore::make_node();
        _box->shape(kaacore::Shape::Box({20., 20.}));
        this->box = this->root_node.add_child(_box);

        auto _cursor = kaacore::make_node();
        _cursor->shape(kaacore::Shape::Circle(3.));
        _cursor->color({0.7, 0.7, 0.7, 1.});
        this->cursor = this->root_node.add_child(_cursor);
    }

    void _mark_box()
    {
        KAACORE_APP_LOG_INFO(" *** Marking. *** ");
        if (this->clicked) {
            this->box->color({1., 1., 1., 1.});
            this->timer_txt->color({1., 1., 1., 1.});
            this->clicked = false;
        } else {
            this->box->color({1., 0.5, 0., 1.});
            this->timer_txt->color({1., 0.5, 0., 1.});
            this->clicked = true;
        }
    }

    void update(const kaacore::Duration dt) override
    {
        for (auto const& event : this->get_events()) {
            if (auto kb_ev = event.keyboard_key()) {
                if (kb_ev->is_key_down() and
                    kb_ev->key() == kaacore::Keycode::space) {
                    this->_mark_box();
                } else if (
                    kb_ev->is_key_down() and
                    kb_ev->key() == kaacore::Keycode::v) {
                    kaacore::get_engine()->vertical_sync(
                        not kaacore::get_engine()->vertical_sync());
                    KAACORE_APP_LOG_INFO(
                        "Vertical sync: {}",
                        kaacore::get_engine()->vertical_sync());
                }
            } else if (auto mc_ev = event.mouse_button()) {
                if (mc_ev->is_button_down() and
                    mc_ev->button() == kaacore::MouseButton::left) {
                    this->_mark_box();
                }
            }
        }
        kaacore::Duration delta = kaacore::Clock::now() - this->start_time;
        this->timer_txt->text.content(fmt::format("{:.6f}", delta.count()));

        this->cursor->position(this->camera().unproject_position(
            kaacore::get_engine()->input_manager->mouse.get_position()));
    }
};

extern "C" int
main(int argc, char* argv[])
{
    kaacore::Engine eng({100, 100});
    InputDemoScene scene;
    scene.camera().position({0., 0.});
    eng.run(&scene);

    return 0;
}
