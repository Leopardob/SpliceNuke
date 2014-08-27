#include <cassert>

#include <DDImage/Knobs.h>
#include <DDImage/SourceGeo.h>
#include <DDImage/PolyMesh.h>

#include <FabricSplice.h>

using namespace DD::Image;

#define FOR_EACH_BOOL(p) for(std::map<std::string, bool>::iterator it=((spliceGeometryOp*)p)->_boolValues.begin(); it!=((spliceGeometryOp*)p)->_boolValues.end(); it++)
#define FOR_EACH_INT(p) for(std::map<std::string, int>::iterator it=((spliceGeometryOp*)p)->_intValues.begin(); it!=((spliceGeometryOp*)p)->_intValues.end(); it++)
#define FOR_EACH_FLOAT(p) for(std::map<std::string, float>::iterator it=((spliceGeometryOp*)p)->_floatValues.begin(); it!=((spliceGeometryOp*)p)->_floatValues.end(); it++)
#define FOR_EACH_VECTOR3(p) for(std::map<std::string, Vector3>::iterator it=((spliceGeometryOp*)p)->_vector3Values.begin(); it!=((spliceGeometryOp*)p)->_vector3Values.end(); it++)


namespace tri 
{
  //
  // Constants
  //

  static const char* kSpliceGeometryClass = "spliceGeometry";
  static unsigned int gInstanceCount = 0;


  //
  // Declarations
  //

  class spliceGeometryOp : public SourceGeo 
  {
    public:
      spliceGeometryOp(Node* node);

      virtual const char* Class() const;
      virtual void knobs(Knob_Callback f);
      virtual int knob_changed(Knob*);

      static Op* Build(Node* node);
      static void addSpliceKnobs(void*, Knob_Callback); 

      static const Op::Description description;

    protected:
      virtual void create_geometry(Scene& scene, GeometryList& out);
      virtual void append(Hash& hash);
      virtual void get_geometry_hash();

    private:
      FabricSplice::DGGraph _graph;
      const char * _fileName;
      std::map<std::string, bool> _boolValues;
      std::map<std::string, int> _intValues;
      std::map<std::string, float> _floatValues;
      std::map<std::string, Vector3> _vector3Values;
      unsigned int _numSpliceKnobs;
  };


  //
  // Implementations
  //

  spliceGeometryOp::spliceGeometryOp(Node* node) :
    SourceGeo(node),
    _fileName(""),
    _numSpliceKnobs(0)
  {
    if(gInstanceCount == 0)
      FabricCore::Initialize();
    _graph = FabricSplice::DGGraph("nukeGraph");
    gInstanceCount++;
  }


  const char* spliceGeometryOp::Class() const
  {
    return kSpliceGeometryClass;
  }


  void spliceGeometryOp::knobs(Knob_Callback f) {
    SourceGeo::knobs(f); // Set up the common SourceGeo knobs.
    File_knob(f, &_fileName, "spliceFile", "spliceFile");
    SetFlags(f, Knob::KNOB_CHANGED_ALWAYS);

    if(!f.makeKnobs())
      spliceGeometryOp::addSpliceKnobs(this->firstOp(), f);
  }

  int spliceGeometryOp::knob_changed(Knob* k)
  {
      if(k->is("spliceFile"))
      {
        _fileName = k->get_text();
        try
        {
          if(_graph.getDGPortCount() == 0)
            _graph.loadFromFile(_fileName);

          for(unsigned int i=0;i<_graph.getDGPortCount();i++)
          {
            FabricSplice::DGPort port = _graph.getDGPort(_graph.getDGPortName(i));
            if(port.getMode() == FabricSplice::Port_Mode_OUT)
              continue;
            std::string portName = port.getName();
            std::string dataType = port.getDataType();
            if(dataType == "Boolean" && port.isArray() == false)
            {
              FabricCore::Variant variant = port.getDefault();
              _boolValues.insert(std::pair<std::string, bool>(portName, variant.getBoolean()));
            }
            else if(dataType == "Integer" && port.isArray() == false)
            {
              FabricCore::Variant variant = port.getDefault();
              _intValues.insert(std::pair<std::string, int>(portName, (int)variant.getSInt32()));
            }
            else if(dataType == "Scalar" && port.isArray() == false)
            {
              FabricCore::Variant variant = port.getDefault();
              _floatValues.insert(std::pair<std::string, float>(portName, (float)variant.getFloat64()));
            }
            else if(dataType == "Vec3" && port.isArray() == false)
            {
              FabricCore::Variant variant = port.getDefault();
              Vector3 value;
              value.x = variant.getDictValue("x")->getFloat64();
              value.y = variant.getDictValue("y")->getFloat64();
              value.z = variant.getDictValue("z")->getFloat64();
              _vector3Values.insert(std::pair<std::string, Vector3>(portName, value));
            }
          }

          _numSpliceKnobs = replace_knobs(knob("spliceFile"), _numSpliceKnobs, addSpliceKnobs, this->firstOp());
          return 1;
        }
        catch(FabricSplice::Exception e)
        {
          Op::error("%s", e.what());
        }
      }
      return SourceGeo::knob_changed(k);
  }

  void spliceGeometryOp::addSpliceKnobs(void* p, Knob_Callback f) {
    FOR_EACH_BOOL(p) Bool_knob(f, &it->second, it->first.c_str());
    FOR_EACH_INT(p) Int_knob(f, &it->second, it->first.c_str());
    FOR_EACH_FLOAT(p) Float_knob(f, &it->second, it->first.c_str());
    FOR_EACH_VECTOR3(p) XYZ_knob(f, &it->second.x, it->first.c_str());
  }

  Op* spliceGeometryOp::Build(Node* node)
  {
    return new spliceGeometryOp(node);
  }


  void spliceGeometryOp::create_geometry(Scene& scene, GeometryList& out)
  {
    try
    {
      if(strlen(_fileName) == 0)
        return;
      if(_graph.getDGPortCount() == 0)
        _graph.loadFromFile(_fileName);

      // try to find the PolygonMesh port
      std::string portName;
      for(unsigned int i=0;i<_graph.getDGPortCount();i++)
      {
        FabricSplice::DGPort port = _graph.getDGPort(_graph.getDGPortName(i));
        if(port.getDataType() != std::string("PolygonMesh"))
          continue;
        if(port.isArray())
          continue;
        if(port.getMode() == FabricSplice::Port_Mode_IN)
          continue;
        portName = port.getName();
        break;
      }

      FabricSplice::DGPort polygonMeshPort = _graph.getDGPort((portName).c_str());
      if(!polygonMeshPort.isValid())
      {
        Op::error("The node '%s' does not contain a PolygonMesh port.", _fileName);
        return;
      }

      FOR_EACH_BOOL(this)
      {
        FabricSplice::DGPort port = _graph.getDGPort(it->first.c_str());
        if(!port.isValid())
          continue;
        if(port.getMode() == FabricSplice::Port_Mode_OUT)
          continue;
        std::string dataType = port.getDataType();
        if(dataType == "Boolean" && port.isArray() == false)
          port.setVariant(FabricCore::Variant::CreateBoolean(it->second));
      }
      FOR_EACH_INT(this)
      {
        FabricSplice::DGPort port = _graph.getDGPort(it->first.c_str());
        if(!port.isValid())
          continue;
        if(port.getMode() == FabricSplice::Port_Mode_OUT)
          continue;
        std::string dataType = port.getDataType();
        if(dataType == "Integer" && port.isArray() == false)
          port.setVariant(FabricCore::Variant::CreateSInt32(it->second));
      }
      FOR_EACH_FLOAT(this)
      {
        FabricSplice::DGPort port = _graph.getDGPort(it->first.c_str());
        if(!port.isValid())
          continue;
        if(port.getMode() == FabricSplice::Port_Mode_OUT)
          continue;
        std::string dataType = port.getDataType();
        if(dataType == "Scalar" && port.isArray() == false)
          port.setVariant(FabricCore::Variant::CreateFloat64(it->second));
      }
      FOR_EACH_VECTOR3(this)
      {
        FabricSplice::DGPort port = _graph.getDGPort(it->first.c_str());
        if(!port.isValid())
          continue;
        if(port.getMode() == FabricSplice::Port_Mode_OUT)
          continue;
        std::string dataType = port.getDataType();
        if(dataType == "Vec3" && port.isArray() == false)
        {
          FabricCore::Variant value = FabricCore::Variant::CreateDict();
          value.setDictValue("x", FabricCore::Variant::CreateFloat64(it->second.x));
          value.setDictValue("y", FabricCore::Variant::CreateFloat64(it->second.y));
          value.setDictValue("z", FabricCore::Variant::CreateFloat64(it->second.z));
          port.setVariant(value);
        }
      }

      std::vector<float> pointsf;
      std::vector<float> normalsf;
      std::vector<int32_t> nsides;
      std::vector<int32_t> vidxs;

      FabricCore::RTVal rtMesh = polygonMeshPort.getRTVal(true);
      if(!rtMesh.isValid() || rtMesh.isNullObject())
      {
        Op::error("The node '%s' has a null object in the PolygonMesh port.", _fileName);
        return;
      }

      unsigned int nbPoints = rtMesh.callMethod("UInt64", "pointCount", 0, 0).getUInt64();
      unsigned int nbPolygons = rtMesh.callMethod("UInt64", "polygonCount", 0, 0).getUInt64();
      unsigned int nbSamples = rtMesh.callMethod("UInt64", "polygonPointsCount", 0, 0).getUInt64();

      pointsf.resize(nbPoints * 3);
      if(pointsf.size() > 0)
      {
        std::vector<FabricCore::RTVal> args(2);
        args[0] = FabricSplice::constructExternalArrayRTVal("Float32", pointsf.size(), &pointsf[0]);
        args[1] = FabricSplice::constructUInt32RTVal(3); // components
        rtMesh.callMethod("", "getPointsAsExternalArray", 2, &args[0]);
      }
      normalsf.resize(nbSamples * 3);
      if(normalsf.size() > 0)
      {
        std::vector<FabricCore::RTVal> args(1);
        args[0] = FabricSplice::constructExternalArrayRTVal("Float32", normalsf.size(), &normalsf[0]);
        rtMesh.callMethod("", "getNormalsAsExternalArray", 1, &args[0]);
      }
      nsides.resize(nbPolygons);
      vidxs.resize(nbSamples);
      if(nsides.size() > 0 && vidxs.size() > 0)
      {
        std::vector<FabricCore::RTVal> args(2);
        args[0] = FabricSplice::constructExternalArrayRTVal("UInt32", nsides.size(), &nsides[0]);
        args[1] = FabricSplice::constructExternalArrayRTVal("UInt32", vidxs.size(), &vidxs[0]);
        rtMesh.callMethod("", "getTopologyAsCountsIndicesExternalArrays", 2, &args[0]);
      }

      int obj = 0;
      // if (rebuild(Mask_Primitives))
      {
        out.delete_objects();
        out.add_object(obj);

        PolyMesh * mesh = new PolyMesh();
        int face[4];

        unsigned int offset = 0;
        for(size_t i=0;i<nsides.size();i++)
        {
          face[0] = vidxs[offset++];
          face[1] = vidxs[offset++];
          face[2] = vidxs[offset++];
          if(nsides[i] == 4)
            face[3] = vidxs[offset++];
          mesh->add_face(nsides[i], face);
        }

        out.add_primitive(obj, mesh);
      }

      // if (rebuild(Mask_Points))
      {
        PointList& points = *out.writable_points(obj);
        unsigned int nbPoints = pointsf.size() / 3;
        points.resize(nbPoints);

        unsigned int offset = 0;
        for(unsigned int i=0;i<nbPoints;i++)
        {
          points[i].x = pointsf[offset++];
          points[i].y = pointsf[offset++];
          points[i].z = pointsf[offset++];
        }
      }

      // if (rebuild(Mask_Attributes))
      {
        Attribute* N = out.writable_attribute(obj, Group_Vertices, "N", NORMAL_ATTRIB);
        unsigned int nbVertices = normalsf.size() / 3;
        unsigned int offset = 0;
        for (unsigned p = 0; p < nbVertices; p++)
        {
          Vector3 normal;
          normal.x = normalsf[offset++];
          normal.y = normalsf[offset++];
          normal.z = normalsf[offset++];
          N->normal(p) = normal;
        }
      }
    }
    catch(FabricSplice::Exception e)
    {
      Op::error("%s", e.what());
      return;
    }
    catch(FabricCore::Exception e)
    {
      Op::error("%s", e.getDesc_cstr());
      return;
    }
  }

  void spliceGeometryOp::append(Hash& hash)
  {
    hash.append(_fileName);
    FOR_EACH_BOOL(this) hash.append(it->second ? 1.7 : 0.0);
    FOR_EACH_INT(this) hash.append((float)it->second);
    FOR_EACH_FLOAT(this) hash.append(it->second);
    FOR_EACH_VECTOR3(this)
    {
      hash.append(it->second.x);
      hash.append(it->second.y);
      hash.append(it->second.z);
    }
  }

  void spliceGeometryOp::get_geometry_hash()
  {
    SourceGeo::get_geometry_hash();
    append(geo_hash[Group_Points]);
  }

  const Op::Description spliceGeometryOp::description(kSpliceGeometryClass, spliceGeometryOp::Build);

} // namespace tri

