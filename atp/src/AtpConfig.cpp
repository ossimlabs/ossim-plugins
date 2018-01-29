//**************************************************************************************************
//
//     OSSIM Open Source Geospatial Data Processing Library
//     See top level LICENSE.txt file for license information
//
//**************************************************************************************************

#include "AtpConfig.h"
#include "AtpOpenCV.h"
#include <ossim/base/ossimCommon.h>
#include <ossim/base/ossimException.h>
#include <ossim/base/ossimPreferences.h>
#include <opencv2/features2d/features2d.hpp>
#include <memory>

using namespace std;

namespace ATP
{
AtpParam AtpConfig::s_nullParam;

AtpParam::AtpParam(const ossimString& argname,
                   const ossimString& arglabel,
                   const ossimString& argdescr,
                   ParamType   argparamType,
                   void* value)
:  _name (argname),
   _label (arglabel),
   _descr (argdescr),
   _type (argparamType),
   _value(0)
{
   setValue(value);
}

AtpParam::AtpParam(const AtpParam& copy)
:  _label (copy._label),
   _name (copy._name),
   _descr (copy._descr),
   _type (copy._type),
   _value (0)
{
   setValue(copy._value);
}

void AtpParam::setValue(void* value)
{
   if (!value)
      return;

   switch (_type)
   {
   case AtpParam::BOOL:
      _value = new bool;
      memcpy(_value, value, sizeof(bool));
      break;
   case AtpParam::INT:
      _value = new int;
      memcpy(_value, value, sizeof(int));
      break;
   case AtpParam::UINT:
      _value = new unsigned int;
      memcpy(_value, value, sizeof(unsigned int));
      break;
   case AtpParam::FLOAT:
      _value = new double;
      memcpy(_value, value, sizeof(double));
      break;
   case AtpParam::STRING:
      _value = new string(*(string*)(value));
      break;
   case AtpParam::VECTOR:
      _value = new vector<double>(*(vector<double>*)(value));
      break;
   default:
      _value = 0;
   }
}

void AtpParam::resetValue()
{
   if (!_value)
      return;

   switch (_type)
   {
   case AtpParam::BOOL:
      delete (bool*)_value;
      break;
   case AtpParam::INT:
   case AtpParam::UINT:
      delete (int*)_value;
      break;
   case AtpParam::FLOAT:
      delete (double*)_value;
      break;
   case AtpParam::STRING:
      delete (string*)_value;
      break;
   case AtpParam::VECTOR:
      ((vector<double>*)_value)->clear();
      delete (vector<double>*)_value;
      break;
   default:
      break;
   }
   _value = 0;
}

bool AtpParam::loadJSON(const Json::Value& paramNode)
{
   try
   {
      _name = paramNode["name"].asString();
      _label = paramNode["label"].asString();
      _descr = paramNode["descr"].asString();
      Json::Value value = paramNode["value"];

      ossimString ptype = paramNode["type"].asString();
      if (ptype.empty() || _name.empty())
         return false;

      ptype.upcase();

      if (ptype == "VECTOR")
      {
         _type = AtpParam::VECTOR;
         vector<double> v;
         if (value.isArray())
         {
            int n = value.size();
            for (unsigned int j=0; j<n; ++j)
               v.push_back(value[j].asDouble());
         }
         setValue(&v);
      }
      else
      {
         // Screen for param value list as found in the default config JSONs. Pick the first element
         // as the default:
         if (value.isArray())
            value = value[0];

         if (ptype == "BOOL")
         {
            _type = AtpParam::BOOL;
            bool v = value.asBool();
            setValue(&v);
         }
         else if (ptype == "UINT")
         {
            _type = AtpParam::UINT;
            unsigned int v = value.asUInt();
            setValue(&v);
         }
         else if (ptype == "INT")
         {
            _type = AtpParam::INT;
            int v = value.asInt();
            setValue(&v);
         }
         else if (ptype == "FLOAT")
         {
            _type = AtpParam::FLOAT;
            double v = value.asDouble();
            setValue(&v);
         }
         else if (ptype == "STRING")
         {
            _type = AtpParam::STRING;
            string v = value.asString();
            setValue(&v);
         }
      }
   }
   catch (exception& e)
   {
      ossimNotify(ossimNotifyLevel_WARN)<<"AtpParam::loadJSON() parse error encountered. Ignoring "
            "but should check the AtpConfig JSON for parameter <"<<_name<<">."<<endl;
      return false;
   }
   return true;
}

void AtpParam::saveJSON(Json::Value& paramNode) const
{
   vector<double>& v = *(vector<double>*)_value; // maybe not used since maybe not valid cast
   unsigned int n = 0;
   int i;
   double f;
   string s;
   bool b;

   paramNode["name"] = _name.string();
   paramNode["label"] = _label.string();
   paramNode["descr"] = _descr.string();

   switch (_type)
   {
   case AtpParam::BOOL:
      paramNode["type"] = "bool";
      b = *(bool*)_value;
      paramNode["value"] = b;
      break;

   case AtpParam::INT:
      paramNode["type"] = "int";
      i = *(int*)_value;
      paramNode["value"] = i;
      break;

   case AtpParam::UINT:
      paramNode["type"] = "uint";
      n = *(unsigned int*)_value;
      paramNode["value"] = n;
      break;

   case AtpParam::FLOAT:
      paramNode["type"] = "float";
      f = *(double*)_value;
      paramNode["value"] = f;
      break;

   case AtpParam::STRING:
      paramNode["type"] = "string";
      s = *(string*)_value;
      paramNode["value"] = s;
      break;

   case AtpParam::VECTOR:
      paramNode["type"] = "vector";
      n = v.size();
      for (unsigned int j=0; j<n; ++j)
         paramNode["value"][j] = v[j];
      break;

   default:
      break;
   }
}

bool AtpParam::asBool() const
{
   if (_type == BOOL)
      return *(bool*)_value;
   return false;
}

unsigned int AtpParam::asUint() const
{
   if (_type == UINT)
      return *(unsigned int*)_value;
   return 0;
}

int  AtpParam::asInt() const
{
   if ((_type == INT) || (_type == UINT))
      return *(int*)_value;
   return 0;
}

double AtpParam::asFloat() const
{
   if (_type == FLOAT)
      return *(double*)_value;
   return ossim::nan();
}

std::string AtpParam::asString() const
{
   if (_type == STRING)
      return *(string*)_value;
   return "";
}

void AtpParam::asVector(std::vector<double>& v) const
{
   v.clear();
   if (_type == VECTOR)
      v = *(vector<double>*)_value;
}

ostream& operator<<(std::ostream& out, const AtpParam& obj)
{
   Json::Value jsonNode;
   obj.saveJSON(jsonNode);
   Json::StyledStreamWriter ssw;
   ssw.write(out, jsonNode);
   return out;
}

//-------------------------------------------------------------

AtpConfig& AtpConfig::instance()
{
   static AtpConfig singleton;
   return singleton;
}

AtpConfig::AtpConfig()
{

   // Register the ISA-common parameters in the params map:
   readConfig();
}
   
AtpConfig::~AtpConfig()
{
   m_paramsMap.clear();
}

bool AtpConfig::readConfig(const string& cn)
{
   // This method could eventually curl a spring config server for the param JSON. For now it
   // is reading the installed share/ossim-isa system directory for config JSON files.

   // The previous parameters list is cleared first for a fresh start:
   m_paramsMap.clear();

   ossimFilename configFilename;
   Json::Reader reader;
   Json::Value jsonRoot;
   try
   {
      // First establish the directory location of the default config files:
      ossimFilename shareDir = ossimPreferences::instance()->
         preferencesKWL().findKey( std::string( "ossim_share_directory" ) );
      shareDir += "/atp";
      if (!shareDir.isDir())
         throw ossimException("Nonexistent share drive provided for config files.");

      // Read the default common parameters first:
      configFilename = "atpConfig.json";
      configFilename.setPath(shareDir);
      ifstream file1 (configFilename.string());
      if (file1.fail())
         throw ossimException("Bad file open.");
      if (!reader.parse(file1, jsonRoot))
         throw ossimException("Bad file JSON parse");
      jsonRoot = jsonRoot["parameters"];
      loadJSON(jsonRoot);
      file1.close();

      // Read the algorithm-specific default parameters if generic algo name specified as config:
      ossimString configName (cn);
      configFilename.clear();
      if (configName == "crosscorr")
         configFilename = "crossCorrConfig.json";
      else if (configName == "descriptor")
         configFilename = "descriptorConfig.json";
      else if (configName == "nasa")
         configFilename = "nasaConfig.json";
      else if (!configName.empty())
      {
         // Custom configuration. TODO: the path here is still the install's share directory.
         // Eventually want to provide a database access to the config JSON.
         configFilename = configName + ".json";
      }

      // Load the specified configuration. This will override the common defaults:
      if (!configFilename.empty())
      {
         configFilename.setPath(shareDir);
         ifstream file2 (configFilename.string());
         if (file2.fail() || !reader.parse(file2, jsonRoot))
            throw ossimException("Bad file open or JSON parse");
         jsonRoot = jsonRoot["parameters"];
         loadJSON(jsonRoot);
         file2.close();
      }
   }
   catch (ossimException& e)
   {
      ossimNotify(ossimNotifyLevel_WARN)<<"AtpConfig::readConfig():  Could not open/parse "
            "config file at <"<< configFilename << ">. Error: "<<e.what()<<endl;
      return false;
   }
   return true;
}

AtpParam& AtpConfig::getParameter(const char* paramName)
{
   map<string, AtpParam>::iterator i = m_paramsMap.find(string(paramName));
   if (i != m_paramsMap.end())
      return i->second;
   return s_nullParam;
}

void AtpConfig::setParameter(const AtpParam& p)
{
   // Tricky stuff to make sure it is a deep copy of the parameter:
   string key = p.name().string();
   std::map<std::string, AtpParam>::iterator iter = m_paramsMap.find(key);
   if (iter != m_paramsMap.end())
      m_paramsMap.erase(key);
   m_paramsMap.emplace(key, p);
}

bool AtpConfig::paramExists(const char* paramName) const
{
   map<string, AtpParam>::const_iterator i = m_paramsMap.find(string(paramName));
   if (i != m_paramsMap.end())
      return true;
   return false;
}

void AtpConfig::loadJSON(const Json::Value& json_node)
{
   Json::Value paramNode;

   // Support two forms: long (with full param descriptions and types), or short (just name: value)
   if (json_node.isArray())
   {
      // Long form:
      for (unsigned int i=0; i<json_node.size(); ++i)
      {
         paramNode = json_node[i];
         AtpParam p;
         if (p.loadJSON(paramNode))
            setParameter(p);
      }
   }
   else
   {
      // Short form expects a prior entry in the params map whose value will be overriden here:
      Json::Value::Members members = json_node.getMemberNames();
      for (size_t i=0; i<members.size(); ++i)
      {
         AtpParam& p = getParameter(members[i].c_str());
         if (p.name().empty())
         {
            ossimNotify(ossimNotifyLevel_WARN)<<"AtpConfig::loadJSON():  Attempted to override "
                  "nonexistent parameter <"<< members[i] << ">. Ignoring request."<<endl;
            continue;
         }
         if (p.descr().contains("DEPRECATED"))
         {
            ossimNotify(ossimNotifyLevel_WARN)<<"AtpConfig::loadJSON()  Parameter "<<p.name()
                  <<" "<<p.descr()<<endl;
            continue;
         }

         // Create a full JSON representation of the named parameter from the default list, replace
         // its value, and recreate the parameter from the updated full JSON:
         p.saveJSON(paramNode);
         paramNode["value"] = json_node[p.name().string()];
         p.loadJSON(paramNode);
     }
   }
}


void AtpConfig::saveJSON(Json::Value& json_node) const
{
   Json::Value paramNode;

   map<string, AtpParam>::const_iterator param = m_paramsMap.begin();
   int entry = 0;
   while (param != m_paramsMap.end())
   {
      param->second.saveJSON(paramNode);
      json_node[entry++] = paramNode;
      ++param;
   }
}

bool AtpConfig::diagnosticLevel(unsigned int level) const
{
   map<string, AtpParam>::const_iterator i = m_paramsMap.find(string("diagnosticLevel"));
   if (i != m_paramsMap.end())
   {
      unsigned int levelSetting = i->second.asUint();
      return (level <= levelSetting);
   }
   return false;
}

std::ostream& operator<<(std::ostream& out, const AtpConfig& obj)
{
   Json::Value configJsonNode;
   obj.saveJSON(configJsonNode);
   Json::StyledStreamWriter ssw;
   ssw.write(out, configJsonNode);

   return out;
}

}
