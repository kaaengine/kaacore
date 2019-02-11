#include "kaacore/scene.h"
#include "kaacore/engine.h"


void Scene::process_frame(uint32_t dt)
{
    this->time += dt;
    this->update(dt);
    // TODO draw nodes
}


void Scene::update(uint32_t dt)
{
}


const std::vector<Event>& Scene::get_events() const
{
    return get_engine()->input_manager->events_queue;
}
