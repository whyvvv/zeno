#include <zeno/NumericObject.h>
#include <zeno/PrimitiveObject.h>
#include <zeno/StringObject.h>
#include <zeno/VDBGrid.h>
#include <zeno/utils/vec.h>
#include <zeno/zeno.h>
#include <zeno/ZenoInc.h>

namespace zeno {

template <class T> struct attr_to_vdb_type {};

template <> struct attr_to_vdb_type<float> {
  static constexpr bool is_scalar = true;
  using type = VDBFloatGrid;
};

template <> struct attr_to_vdb_type<vec3f> {
  static constexpr bool is_scalar = false;
  using type = VDBFloat3Grid;
};

template <> struct attr_to_vdb_type<int> {
  static constexpr bool is_scalar = true;
  using type = VDBIntGrid;
};

template <> struct attr_to_vdb_type<vec3i> {
  static constexpr bool is_scalar = false;
  using type = VDBInt3Grid;
};

template <class T>
void sampleVDBAttribute(std::vector<vec3f> const &pos, std::vector<T> &arr,
                        VDBGrid *ggrid) {
  using VDBType = typename attr_to_vdb_type<T>::type;
  auto ptr = dynamic_cast<VDBType *>(ggrid);
  if (!ptr) {
    printf("ERROR: vdb attribute type mismatch!\n");
    return;
  }
  auto grid = ptr->m_grid;

#pragma omp parallel for
  for (int i = 0; i < pos.size(); i++) {
    auto p0 = pos[i];
    auto p1 = vec_to_other<openvdb::Vec3R>(p0);
    auto p2 = grid->worldToIndex(p1);
    auto val = openvdb::tools::BoxSampler::sample(grid->tree(), p2);
    if constexpr (attr_to_vdb_type<T>::is_scalar) {
      arr[i] = val;
    } else {
      arr[i] = other_to_vec<3>(val);
    }
  }
}

struct SampleVDBToPrimitive : INode {
  virtual void apply() override {
    auto prim = get_input<PrimitiveObject>("prim");
    auto grid = get_input<VDBGrid>("vdbGrid");
    auto attr = get_input<StringObject>("primAttr")->get();
    auto &pos = prim->attr<vec3f>("pos");

    if (dynamic_cast<VDBFloatGrid *>(grid.get()))
        prim->add_attr<float>(attr);
    else if (dynamic_cast<VDBFloat3Grid *>(grid.get()))
        prim->add_attr<vec3f>(attr);
    else
        printf("unknown vdb grid type\n");

    std::visit([&](auto &vel) { sampleVDBAttribute(pos, vel, grid.get()); },
               prim->attr(attr));

    set_output("prim", std::move(prim));
  }
};

ZENDEFNODE(SampleVDBToPrimitive, {
                                     {"prim", "vdbGrid", "primAttr"},
                                     {"prim"},
                                     {},
                                     {"openvdb"},
                                 });


struct TraceOneStep : INode {
  virtual void apply() override {
    auto steps = get_input<NumericObject>("steps")->get<int>();
    auto prim = get_input<PrimitiveObject>("prim");
    auto vecField = get_input<VDBFloat3Grid>("vecField");
    auto size = get_input<NumericObject>("size")->get<int>();
    auto dt = get_input<NumericObject>("dt")->get<float>();
    auto maxlength = std::numeric_limits<float>::infinity();
    if(has_input("maxlength"))
    {
      maxlength = get_input<NumericObject>("maxlength")->get<float>();
    }
    
    for(auto s=0;s<steps;s++){
      prim->resize(prim->size()+size);
      auto &pos = prim->attr<vec3f>("pos");
      auto &lengtharr = prim->attr<float>("length");
      auto &velarr = prim->attr<vec3f>("vel");
      prim->lines.resize(prim->lines.size() + size);

      #pragma omp parallel for
      for(int i=prim->size()-size; i<prim->size(); i++)
      {
        auto p0 = pos[i-size];
        
        auto p1 = vec_to_other<openvdb::Vec3R>(p0);
        auto p2 = vecField->worldToIndex(p1);
        auto vel = openvdb::tools::BoxSampler::sample(vecField->m_grid->tree(), p2);
        velarr[i-size] = other_to_vec<3>(vel);
        auto pend = p0;
        if(lengtharr[i-size]<maxlength && maxlength>0)
        {
            pend += dt * other_to_vec<3>(vel);
        }
        pos[i] = pend;
        velarr[i] = velarr[i-size];
        lengtharr[i] = lengtharr[i-size] + length(pend - p0);
        prim->lines[i-size] = zeno::vec2i(i-size, i);
      }
    }
    

    
    
    set_output("prim", std::move(prim));
  }
};

ZENDEFNODE(TraceOneStep, {
                                     {"prim", "dt", "size", "steps", "maxlength", "vecField"},
                                     {"prim"},
                                     {},
                                     {"openvdb"},
                                 });


struct GetVDBBound : INode {
  virtual void apply() override {
    auto grid = get_input<VDBGrid>("vdbGrid");
    auto bbmin = zeno::IObject::make<zeno::NumericObject>();
    auto bbmax = zeno::IObject::make<zeno::NumericObject>();

    zeno::vec3f bmin, bmax;
    openvdb::CoordBBox box = grid->evalActiveVoxelBoundingBox();
    auto corner = box.min();
    auto length = box.max() - box.min();
    auto world_min = grid->indexToWorld(box.min());
    auto world_max = grid->indexToWorld(box.max());

    for (size_t d = 0; d < 3; d++) {
      bmin[d] = world_min[d];
      bmax[d] = world_max[d];
    }

    for (int dx = 0; dx < 2; dx++)
      for (int dy = 0; dy < 2; dy++)
        for (int dz = 0; dz < 2; dz++) {
          auto coord =
              corner + decltype(length){dx ? length[0] : 0, dy ? length[1] : 0,
                                        dz ? length[2] : 0};

          auto pos = grid->indexToWorld(coord);

          for (int d = 0; d < 3; d++) {
            bmin[d] = pos[d] < bmin[d] ? pos[d] : bmin[d];
            bmax[d] = pos[d] > bmax[d] ? pos[d] : bmax[d];
          }
        }

    bbmin->set<zeno::vec3f>(bmin);
    bbmax->set<zeno::vec3f>(bmax);
    set_output("bmin", bbmin);
    set_output("bmax", bbmax);
  }
};

ZENDEFNODE(GetVDBBound, {
                            {"vdbGrid"},
                            {"bmin", "bmax"},
                            {},
                            {"openvdb"},
                        });


} // namespace zeno
