#pragma once

#include <Std/Vector.hpp>
#include <Std/StringBuilder.hpp>

namespace Std
{
    template<typename T, usize Size>
    class CircularQueue {
    public:
        CircularQueue()
        {
            m_offset = 0;
            m_size = 0;
        }
        ~CircularQueue()
        {
            clear();
        }
        CircularQueue(CircularQueue&& other)
        {
            m_size = 0;
            m_offset = 0;

            *this = move(other);
        }

        void dump()
        {
            StringBuilder builder;

            builder.append("[ ");

            const char *prefix = "";
            for (usize index = 0; index < m_size; ++index) {
                usize offset = (m_offset - m_size + index + Size) % Size;
                auto& target = *reinterpret_cast<T*>(m_data + offset * sizeof(T));

                builder.append(prefix);
                builder.appendf("%", target);

                prefix = ", ";
            }

            builder.append(" ]");

            dbgln("{}", builder);
        }

        T& enqueue(const T& value)
        {
            return enqueue_impl(value);
        }
        T& enqueue(T&& value)
        {
            return enqueue_impl(move(value));
        }

        T& enqueue_front(const T& value)
        {
            return enqueue_front_impl(value);
        }
        T& enqueue_front(T&& value)
        {
            return enqueue_front_impl(move(value));
        }

        T& front()
        {
            ASSERT(m_size > 0);

            usize offset = (m_offset - m_size + Size) % Size;
            return *reinterpret_cast<T*>(m_data + offset * sizeof(T));
        }
        T& back()
        {
            ASSERT(m_size > 0);

            usize offset = (m_offset - 1 + Size) % Size;
            return *reinterpret_cast<T*>(m_data + offset * sizeof(T));
        }

        T& operator[](usize index)
        {
            ASSERT(m_size > index);

            usize offset = (m_offset - m_size + index + Size) % Size;
            return *reinterpret_cast<T*>(m_data + offset * sizeof(T));
        }

        T dequeue()
        {
            auto& target = front();

            T value = move(target);
            target.~T();

            --m_size;

            return value;
        }

        usize size() const { return m_size; }

        usize capacity() const { return Size; }
        usize avaliable() const { return capacity() - size(); }

        void clear()
        {
            usize count = m_size;
            for (usize i = 0; i < count; ++i) {
                front().~T();
                --m_size;
            }
        }

        CircularQueue& operator=(CircularQueue&& other)
        {
            clear();

            usize count = other.m_size;
            for (usize i = 0; i < count; ++i)
                enqueue(other.dequeue());
            ASSERT(other.m_size == 0);

            return *this;
        }

    private:
        template<typename T_>
        T& enqueue_impl(T_&& value)
        {
            ASSERT(m_size < Size);
            ASSERT(m_offset < Size);

            new (m_data + m_offset * sizeof(T)) T { forward<T_>(value) };
            m_offset = (m_offset + 1) % Size;

            ++m_size;

            return back();
        }

        template<typename T_>
        T& enqueue_front_impl(T_&& value)
        {
            ASSERT(m_size < Size);
            ASSERT(m_offset < Size);

            usize offset = (m_offset - m_size - 1 + Size) % Size;

            new (m_data + offset * sizeof(T)) T { forward<T_>(value) };

            ++m_size;

            return front();
        }

        usize m_offset;
        usize m_size;
        u8 m_data[sizeof(T) * Size];
    };
}
