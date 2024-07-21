#include "kaacore/engine.h"
#include "kaacore/log.h"
#include "kaacore/nodes.h"
#include "kaacore/scenes.h"

struct TestDemoScene : kaacore::Scene {
    kaacore::NodePtr node;

    TestDemoScene()
    {
        this->camera().position({0., 0.});

        auto shape_repr = kaacore::make_node();
        auto shape_repr2 = kaacore::make_node();

        shape_repr->shape(kaacore::Shape::Box({150, 150}));
        shape_repr->position(glm::dvec2(0, 0));
        shape_repr->indexable(true);

        shape_repr2->shape(kaacore::Shape::Box({150, 150}));
        shape_repr2->position(glm::dvec2(75, 75));
        shape_repr2->color({1., 0, 0, 1.});
        shape_repr2->indexable(true);

        shape_repr->add_child(shape_repr2);
        this->node = this->root_node.add_child(shape_repr);
    }

    void update(const kaacore::Duration dt) override
    {
        for (auto const& event : this->get_events()) {
            if (auto mouse_button = event.mouse_button()) {
                if (not mouse_button->is_button_down()) {
                    continue;
                }

                auto pos = mouse_button->position();
                if (mouse_button->button() == kaacore::MouseButton::left) {
                    pos = this->camera().unproject_position(pos);
                    auto query = this->spatial_index.query_point(pos);
                    KAACORE_APP_LOG_INFO("Number of nodes: {}", query.size());
                } else if (mouse_button->button() ==
                           kaacore::MouseButton::right) {
                    pos = this->camera().unproject_position(pos);
                    this->node->position(pos);
                }
            }
        }
    }
};

extern "C" int
main(int argc, char* argv[])
{
    kaacore::Engine eng(
        {800, 600}, kaacore::VirtualResolutionMode::aggresive_stretch
    );
    TestDemoScene scene;
    eng.run(&scene);

    return 0;
}
