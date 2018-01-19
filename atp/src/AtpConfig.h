//**************************************************************************************************
//
//     OSSIM Open Source Geospatial Data Processing Library
//     See top level LICENSE.txt file for license information
//
//**************************************************************************************************

#pragma once

#include <ostream>
#include <ossim/plugin/ossimPluginConstants.h>
#include <ossim/base/ossimString.h>
#include <ossim/base/JsonInterface.h>
#include <vector>
#include <map>

namespace ATP
{
// Forward decl defined after AtpParam
class AtpParam;

/**
 * Singleton class maintaining parameters affecting the automatic tie point generation.
 * The state is imported and exported via JSON. There are default configuration files that must
 * be part of the install, that are accessed by this class. Custom settings can also be st
 */
class OSSIM_PLUGINS_DLL AtpConfig : public ossim::JsonInterface
{
public:
   //! Singleton implementation.
   static AtpConfig& instance();

   //! Destructor
   virtual ~AtpConfig();

   bool readConfig(const std::string& configName="");

   //! Reads the params controlling the process from the JSON node named "parameters".
   virtual void loadJSON(const Json::Value& params_json_node);

   //! Reads the params controlling the process from the JSON node named "parameters".
   virtual void saveJSON(Json::Value& params_json_node) const;

   //! Returns a parameter (might be a null parameter if paramName not found in the configuration.
   AtpParam& getParameter(const char* paramName);

   /** Adds parameter to the configuration. Any previous parameter of the same name is replaced. */
   void setParameter(const AtpParam& p);

   //! Convenience method returns TRUE if the currently set diagnostic level is <= level
   bool diagnosticLevel(unsigned int level) const;

   //! Outputs JSON to output stream provided.
   friend std::ostream& operator<<(std::ostream& out, const AtpConfig& obj);

   bool paramExists(const char* paramName) const;

private:
   AtpConfig();
   AtpConfig(const AtpConfig& /*hide_this*/) {}

   bool getBoolValue(bool& rtn_val, const std::string& json_value) const;

   std::map<std::string, AtpParam> m_paramsMap;
   static AtpParam s_nullParam;
};


/**
 * Represents a single configuration parameter. This class provides for packing and unpacking the
 * parameter from JSON payload, and provides for handling all datatypes of parameters.
 */
class AtpParam
{
public:
   enum ParamType {
      UNASSIGNED=0,
      BOOL=1,
      INT=2,
      UINT=3,
      FLOAT=4,
      STRING=5,
      VECTOR=6
   };

   AtpParam() : _type(UNASSIGNED), _value(0) {}

   AtpParam(const ossimString& argname,
            const ossimString& arglabel,
            const ossimString& argdescr,
            ParamType   argparamType,
            void* value);

   AtpParam(const AtpParam& copy);

   ~AtpParam() { resetValue(); }

   /** Initializes from a JSON node. Return true if successful */
   bool loadJSON(const Json::Value& json_node);

   void saveJSON(Json::Value& json_node) const;

   const ossimString& name() const { return _name; }
   const ossimString& label() const { return _label; }
   const ossimString& descr() const { return _descr; }
   ParamType    type() const { return _type; }

   bool isBool() const  {return (_type == BOOL); }
   bool asBool() const;

   bool isUint() const  {return (_type == UINT); }
   unsigned int asUint() const;

   bool isInt() const  {return (_type == INT);}
   int  asInt() const;

   bool isFloat() const {return (_type == FLOAT);}
   double asFloat() const;

   bool isString() const {return (_type == STRING);}
   std::string asString() const;

   bool isVector() const {return (_type == VECTOR);}
   void asVector(std::vector<double>& v) const;

   bool operator==(const AtpParam& p) const { return (p._name == _name); }

   void setValue(const Json::Value& json_node);
   void setValue(void* value);
   void resetValue();

   /** Outputs JSON to output stream provided */
   friend std::ostream& operator<<(std::ostream& out, const AtpParam& obj);

private:

   ossimString _name;
   ossimString _label;
   ossimString _descr;
   ParamType   _type;
   void*       _value;
};


}
