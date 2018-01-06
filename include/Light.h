#ifndef VT_LIGHT_H_
#define VT_LIGHT_H_

#include <TransformObject.h>
#include <glm/glm.hpp>
#include <string>

namespace vt {

class Light : public TransformObject
{
public:
    Light(std::string name   = "",
          glm::vec3   origin = glm::vec3(0),
          glm::vec3   color  = glm::vec3(1));

    const glm::vec3 get_color() const
    {
        return m_color;
    }

    bool is_enabled() const
    {
        return m_enabled;
    }
    void set_enabled(bool enabled)
    {
        m_enabled = enabled;
    }

private:
    glm::vec3 m_color;
    bool      m_enabled;

    void update_transform();
};

}

#endif
