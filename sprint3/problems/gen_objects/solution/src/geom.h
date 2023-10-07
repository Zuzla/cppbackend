#include <iostream>
#include <optional>
#include <cmath>

namespace geom
{
    struct Rect
    {
        double x, y;
        double w, h;
    };

    struct Circle
    {
        double x, y;
        double r;
    };

    struct LineSegment
    {
        // Предполагаем, что x1 <= x2
        double x1, x2;
    };

    bool CheckCirclesForIntersection(Circle c1, Circle c2)
    {
        // Вычислим расстояние между точками функцией std::hypot
        return std::hypot(c1.x - c2.x, c1.y - c2.y) <= c1.r + c2.r;
    }

    std::optional<LineSegment> Intersect(LineSegment s1, LineSegment s2)
    {
        double left = std::max(s1.x1, s2.x1);
        double right = std::min(s1.x2, s2.x2);

        if (right < left)
        {
            return std::nullopt;
        }

        // Здесь использована возможность C++-20 - объявленные
        // инициализаторы (designated initializers).
        // Узнать о ней подробнее можно на сайте cppreference:
        // https://en.cppreference.com/w/cpp/language/aggregate_initialization#Designated_initializers
        return LineSegment{.x1 = left, .x2 = right};
    }

    // Вычисляем проекции на оси
    LineSegment ProjectX(Rect r)
    {
        return LineSegment{.x1 = r.x, .x2 = r.x + r.w};
    }

    LineSegment ProjectY(Rect r)
    {
        return LineSegment{.x1 = r.y, .x2 = r.y + r.h};
    }

    std::optional<Rect> Intersect(Rect r1, Rect r2)
    {
        auto px = Intersect(ProjectX(r1), ProjectX(r2));
        auto py = Intersect(ProjectY(r1), ProjectY(r2));

        if (!px || !py)
        {
            return std::nullopt;
        }

        // Составляем из проекций прямоугольник
        return Rect{.x = px->x1, .y = py->x1, .w = px->x2 - px->x1, .h = py->x2 - py->x1};
    }

    struct Point
    {
        double x, y;
    };

    struct CollectionResult
    {
        bool IsCollected(double collect_radius) const
        {
            return proj_ratio >= 0 &&
                   proj_ratio <= 1 &&
                   sq_distance <= collect_radius * collect_radius;
        }

        // Квадрат расстояния до точки
        double sq_distance;
        // Доля пройденного отрезка
        double proj_ratio;
    };

    // Движемся из точки a в точку b и пытаемся подобрать точку c.
    // Функция корректно работает только при условии ненулевого перемещения.
    CollectionResult TryCollectPoint(Point a, Point b, Point c)
    {
        // Проверим, что перемещение ненулевое.
        // Тут приходится использовать строгое равенство, а не приближённое,
        // пскольку при сборе заказов придётся учитывать перемещение даже на небольшое
        // расстояние.
        if (b.x != a.x || b.y != a.y)
            return;
            
        const double u_x = c.x - a.x;
        const double u_y = c.y - a.y;
        const double v_x = b.x - a.x;
        const double v_y = b.y - a.y;
        const double u_dot_v = u_x * v_x + u_y * v_y;
        const double u_len2 = u_x * u_x + u_y * u_y;
        const double v_len2 = v_x * v_x + v_y * v_y;
        const double proj_ratio = u_dot_v / v_len2;
        const double sq_distance = u_len2 - (u_dot_v * u_dot_v) / v_len2;

        return CollectionResult(sq_distance, proj_ratio);
    }

}