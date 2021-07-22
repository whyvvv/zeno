#include <zeno/zeno.h>
#include <zeno/NumericObject.h>

namespace {

using namespace zeno;

struct MakeOrthonormalBase : INode {
    virtual void apply() override {
        auto normal = get_input<NumericObject>("normal")->get<vec3f>();
        normal = normalize(normal);
        vec3f tangent, bitangent;
        if (has_input("tangent")) {
            tangent = get_input<NumericObject>("tangent")->get<vec3f>();
            bitangent = cross(normal, tangent);
        } else {
            tangent = vec3f(0, 0, 1);
            bitangent = cross(normal, tangent);
            if (dot(bitangent, bitangent) < 1e-5) {
                tangent = vec3f(0, 1, 0);
               bitangent = cross(normal, tangent);
            }
        }
        bitangent = normalize(bitangent);
        tangent = cross(bitangent, normal);

        set_output("normal", std::make_shared<NumericObject>(normal));
        set_output("tangent", std::make_shared<NumericObject>(tangent));
        set_output("bitangent", std::make_shared<NumericObject>(bitangent));
    }
};

ZENDEFNODE(MakeOrthonormalBase, {
    {"normal", "tangent"},
    {"normal", "tangent", "bitangent"},
    {},
    {"math"},
});


struct UnpackNumericVec : INode {
    virtual void apply() override {
        auto vec = get_input<NumericObject>("vec")->value;
        NumericValue x = 0, y = 0, z = 0, w = 0;
        std::visit([&x, &y, &z, &w] (auto const &vec) {
            using T = std::decay_t<decltype(vec)>;
            if constexpr (!is_vec_v<T>) {
                x = vec;
            } else {
                if constexpr (is_vec_n<T> > 0) x = vec[0];
                if constexpr (is_vec_n<T> > 1) y = vec[1];
                if constexpr (is_vec_n<T> > 2) z = vec[2];
                if constexpr (is_vec_n<T> > 3) w = vec[3];
            }
        }, vec);
        set_output("X", std::make_shared<NumericObject>(x));
        set_output("Y", std::make_shared<NumericObject>(y));
        set_output("Z", std::make_shared<NumericObject>(z));
        set_output("W", std::make_shared<NumericObject>(w));
    }
};

ZENDEFNODE(UnpackNumericVec, {
    {"vec"},
    {"X", "Y", "Z", "W"},
    {},
    {"numeric"},
}); // TODO: add PackNumericVec too.


struct NumericRandom : INode {
    virtual void apply() override {
        auto value = std::make_shared<NumericObject>();
        auto dim = get_param<int>("dim");
        if (dim == 1) {
            value->set(float(drand48()));
        } else if (dim == 2) {
            value->set(zeno::vec2f(drand48(), drand48()));
        } else if (dim == 3) {
            value->set(zeno::vec3f(drand48(), drand48(), drand48()));
        } else if (dim == 4) {
            value->set(zeno::vec4f(drand48(), drand48(), drand48(), drand48()));
        } else {
            printf("invalid dim for NumericRandom: %d\n", dim);
        }
        set_output("value", std::move(value));
    }
};

ZENDEFNODE(NumericRandom, {
    {},
    {"value"},
    {{"int", "dim", "1"}},
    {"numeric"},
});


}
