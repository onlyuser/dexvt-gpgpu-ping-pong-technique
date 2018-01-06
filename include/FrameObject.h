#ifndef VT_FRAME_OBJECT_H_
#define VT_FRAME_OBJECT_H_

#include <glm/glm.hpp>

namespace vt {

template<class T, class T2>
class FrameObject
{
public:
    T get_offset() const
    {
        return m_offset;
    }
    T get_dim() const
    {
        return m_dim;
    }
    T2 get_left() const
    {
        return m_offset.x;
    }
    T2 get_bottom() const
    {
        return m_offset.y;
    }
    T2 get_width() const
    {
        return m_dim.x;
    }
    T2 get_height() const
    {
        return m_dim.y;
    }
    float get_aspect_ratio() const
    {
        return static_cast<float>(m_dim.x) / m_dim.y;
    }
    void resize(T2 left, T2 bottom, T2 width, T2 height)
    {
        m_offset.x = left;
        m_offset.y = bottom;
        m_dim.x = width;
        m_dim.y = height;
    }

protected:
    T m_offset;
    T m_dim;

    FrameObject(
            T offset,
            T dim)
        : m_offset(offset),
          m_dim(dim)
    {
    }
    virtual ~FrameObject()
    {
    }
};

}

#endif
