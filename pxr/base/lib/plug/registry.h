//
// Copyright 2016 Pixar
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification; you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
//
#ifndef PLUG_REGISTRY_H
#define PLUG_REGISTRY_H

#include "pxr/base/plug/api.h"

#include "pxr/base/js/value.h"
#include "pxr/base/tf/declarePtrs.h"
#include "pxr/base/tf/hash.h"
#include "pxr/base/tf/hashset.h"
#include "pxr/base/tf/refPtr.h"
#include "pxr/base/tf/singleton.h"
#include "pxr/base/tf/type.h"
#include "pxr/base/tf/weakBase.h"
#include "pxr/base/tf/weakPtr.h"

#include <boost/noncopyable.hpp>
#include <boost/scoped_ptr.hpp>

#include <mutex>
#include <string>
#include <vector>

TF_DECLARE_WEAK_PTRS(PlugPlugin);
struct Plug_RegistrationMetadata;

/// \class PlugRegistry
///
/// Defines an interface for registering plugins.
///
/// PlugRegistry maintains a registry of plug-ins known to the system
/// and provides an interface for base classes to load any plug-ins required
/// to instantiate a subclass of a given type.
///
/// <h2>Defining a Base Class API</h2>
///
/// In order to use this facility you will generally provide a library 
/// which defines the API for a plug-in base class.  This API
/// will be sufficient for the application or framework to make use of
/// custom subclasses that will be written by plug-in developers.
///
/// For example, if you have an image processing application, you might
/// want to support plug-ins that implement image filters.  You can define
/// an abstract base class for image filters that declares the API your 
/// application will require image filters to implement; perhaps something
/// simple like \ref plug_cppcode_PlugRegistry1 "C++ Code Example 1" (Doxygen only).
///
/// People writing custom filters would write a subclass of ImageFilter that
/// overrides the two methods, implementing their own special filtering 
/// behavior.
///
/// \section plug_EnablingPlugins Enabling Plug-in Loading for the Base Class
///
/// In order for ImageFilter to be able to load plug-ins that implement
/// these custom subclasses, it must be registered with the TfType system.
///
/// The ImageFilter base class, as was mentioned earlier, should be made
/// available in a library that the application links with.  This is done
/// so that plug-ins that want to provide ImageFilters can also link with
/// the library allowing them to subclass ImageFilter.
///
/// \section plug_RegisteringPlugins Registering Plug-ins
///
/// A plug-in developer can now write plug-ins with ImageFilter subclasses.
/// Plug-ins can be implemented either as native dynamic libraries (either
/// regular dynamic libraries or framework bundles) or as Python modules.
///
/// Plug-ins must be registered with the registry.  All plugins are
/// registered via RegisterPlugins().  Plug-in Python modules must be
/// directly importable (in other words they must be able to be found in
/// Python's module path.)  Plugins are registered by providing a path or
/// paths to JSON files that describe the location, structure and contents
/// of the plugin.  The standard name for these files in plugInfo.json.
///
/// Typically, the application that hosts plug-ins will locate and register 
/// plug-ins at startup.
///
/// The plug-in facility is lazy.  It does not dynamically load code from 
/// plug-in bundles until that code is required. 
///
/// \section plug_plugInfo plugInfo.json
///
/// A plugInfo.json file has the following structure:
///
/// \code
/// {
///     # Comments are allowed and indicated by a hash at the start of a
///     # line or after spaces and tabs.  They continue to the end of line.
///     # Blank lines are okay, too.
///
///     # This is optional.  It may contain any number of strings.
///     #   Paths may be absolute or relative.
///     #   Paths ending with slash have plugInfo.json appended automatically.
///     #   '*' may be used anywhere to match any character except slash.
///     #   '**' may be used anywhere to match any character including slash.
///     "Includes": [
///         "/absolute/path/to/plugInfo.json",
///         "/absolute/path/to/custom.filename",
///         "/absolute/path/to/directory/with/plugInfo/",
///         "relative/path/to/plugInfo.json",
///         "relative/path/to/directory/with/plugInfo/",
///         "glob*/pa*th/*to*/*/plugInfo.json",
///         "recursive/pa**th/**/"
///     ],
///
///     # This is optional.  It may contain any number of objects.
///     "Plugins": [
///         {
///             # Type is required and may be "library", "python" or "resource".
///             "Type": "library",
///
///             # Name is required.  It should be the Python module name,
///             # the shared library name, or a unique resource name.
///             "Name": "myplugin",
///
///             # Root is optional.  It defaults to ".".
///             # This gives the path to the plugin as a whole if the plugin
///             # has substructure.  For Python it should be the directory
///             # with the __init__.py file.  The path is usually relative.
///             "Root": ".",
///
///             # LibraryPath is required by Type "library" and unused
///             # otherwise.  It gives the path to the shared library
///             # object, either absolute or relative to Root.
///             "LibraryPath": "libmyplugin.so",
///
///             # ResourcePath is option.  It defaults to ".".
///             # This gives the path to the plugin's resources directory.
///             # The path is either absolute or relative to Root.
///             "ResourcePath": "resources",
///
///             # Info is required.  It's described below.
///             "Info": {
///                 # Plugin contents.
///             }
///         }
///     ]
/// }
/// \endcode
///
/// As a special case, if a plugInfo.json contains an object that doesn't
/// have either the "Includes" or "Plugins" keys then it's as if the object
/// was in a "Plugins" array.
///
/// \section plug_Advertising Advertising a Plug-in's Contents
///
/// Once the the plug-ins are registered, the plug-in facility must also be 
/// able to tell what they contain.  Specifically, it must be able to find 
/// out what subclasses of what plug-in base classes each plug-in contains.
/// Plug-ins must advertise this information through their plugInfo.json file
/// in the "Info" key.  In the "Info" object there should be a key "Types"
/// holding an object.
///
/// This "Types" object's keys are names of subclasses and its values are yet
/// more objects (the subclass meta-data objects).  The meta-data objects can
/// contain arbitrary key-value pairs. The plug-in mechanism will look for a
/// meta-data key called "displayName" whose value should be the display name
/// of the subclass.  The plug-in mechanism will look for a meta-data key
/// called "bases" whose value should be an array of base class type names.
///
/// For example, a bundle that contains a subclass of ImageFilter might have
/// a plugInfo.json that looks like the the following example.
///
/// \code
/// {
///     "Types": {
///         "MyCustomCoolFilter" : {
///             "bases": ["ImageFilter"],
///             "displayName": "Add Coolness to Image"
///             # other arbitrary metadata for MyCustomCoolFilter here
///         }
///     }
/// }
/// \endcode
///
/// What this says is that the plug-in contains a type called MyCustomCoolFilter
/// which has a base class ImageFilter and that this subclass should be called
/// "Add Coolness to Image" in user-visible contexts.
///
/// In addition to the "displayName" meta-data key which is actually 
/// known to the plug-in facility, you may put whatever other information
/// you want into a class' meta-data dictionary.  If your plug-in base class
/// wants to define custom keys that it requires all subclasses to provide,
/// you can do that.  Or, if a plug-in writer wants to define their own keys
/// that their code will look for at runtime, that is OK as well.
///
/// \section plug_subClasses Working with Subclasses of a Plug-in Base Class
///
/// Most code with uses types defined in plug-ins doesn't deal with
/// the Plug API directly.  Instead, the TfType interface is used
/// to lookup types and to manufacture instances.  The TfType interface
/// will take care to load any required plugins.
///
/// To wrap up our example, the application that wants to actually use
/// ImageFilter plug-ins would probably do a couple of things.  First, it
/// would get a list of available ImageFilters to present to the user.
/// This could be accomplished as shown in
/// \ref plug_cppcode_PlugRegistry2 "Python Code Example 2" (Doxygen only).
///
/// Then, when the user picks a filter from the list, it would manufacture
/// and instance of the filter as shown in
/// \ref plug_cppcode_PlugRegistry3 "Python Code Example 3" (Doxygen only).
///
/// As was mentioned earlier, this plug-in facility tries to be as lazy 
/// as possible about loading the code associated with plug-ins.  To that end,
/// loading of a plugin will be deferred until an instance of a type
/// is manufactured which requires the plugin.
///
/// \section plug_MultipleSubclasses Multiple Subclasses of Multiple Plug-in Base Classes
///
/// It is possible for a bundle to implement multiple subclasses
/// for a plug-in base class if desired.  If you want to package half a dozen
/// ImageFilter subclasses in one bundle, that will work fine.  All must 
/// be declared in the plugInfo.json.
///
/// It is possible for there to be multiple classes in your
/// application or framework that are plug-in base classes.  Plug-ins that 
/// implement subclasses of any of these base classes can all coexist.  And,
/// it is possible to have subclasses of multiple plug-in base classes in the
/// same bundle.
///
/// When putting multiple subclasses (of the same or different base classes)
/// in a bundle, keep in mind that dynamic loading happens for the whole bundle
/// the first time any subclass is needed, the whole bundle will be loaded.
/// But this is generally not a big concern.
///
/// For example, say the example application also has a plug-in base class
/// "ImageCodec" that allows people to add support for reading and writing
/// other image formats.  Imagine that you want to supply a plug-in that
/// has two codecs and a filter all in a single plug-in.  Your plugInfo.json 
/// "Info" object might look something like this example.
///
/// \code
/// {
///     "Types": {
///         "MyTIFFCodec": {
///             "bases": ["ImageCodec"],
///             "displayName": "TIFF Image"
///         },
///         "MyJPEGCodec": {
///             "bases": ["ImageCodec"],
///             "displayName": "JPEG Image"
///         },
///         "MyCustomCoolFilter" : {
///             "bases": ["ImageFilter"],
///             "displayName": "Add Coolness to Image"
///         }
///     }
/// }
/// \endcode
///
/// \section plug_Dependencies Dependencies on Other Plug-ins
///
/// If you write a plug-in that has dependencies on another plug-in that you
/// cannot (or do not want to) link against statically, you can declare
/// the dependencies in your plug-in's plugInfo.json .  A plug-in declares
/// dependencies on other classes with a PluginDependencies key whose value
/// is a dictionary.  The keys of the dictionary are plug-in base class names
/// and the values are arrays of subclass names.
///
/// The following example contains an example of a plug-in that depends on two
/// classes from the plug-in in the previous example.
///
/// \code
/// {
///     "Types": {
///         "UltraCoolFilter": {
///             "bases": ["MyCustomCoolFilter"],
///             "displayName": "Add Unbelievable Coolness to Image"
///             # A subclass of MyCustomCoolFilter that also uses MyTIFFCodec
///         }
///     },
///     "PluginDependencies": {
///         "ImageFilter": ["MyCustomCoolFilter"],
///         "ImageCodec": ["MyTIFFCodec"]
///     }
/// }
/// \endcode
///
/// The ImageFilter provided by the plug-in in this example depends on the
/// other ImageFilter MyCoolImageFilter and the ImageCodec MyTIFFCodec.
/// Before loading this plug-in, the plug-in facility will ensure that those
/// two classes are present, loading the plug-in that contains them if needed.
///
/// \section plug_cppcode_PlugRegistry1 C++ Code Example 1
/// \code
/// // Declare a base class interface
/// class ImageFilter {
///    public:
///    virtual bool CanFilterImage(const ImagePtr & inputImage) = 0;
///    virtual ImagePtr FilterImage(const ImagePtr & inputImage) = 0;
/// };
/// \endcode
///
/// \section plug_cppcode_PlugRegistry2 Python Code Example 2
/// \code
/// # Get the names of derived types
/// baseType = Tf.Type.Find(ImageFilter)
/// if baseType:
///     derivedTypes = baseType.GetAllDerived()
///     derivedTypeNames = [ derived.typeName for derived in derivedTypes ]
/// \endcode
///
/// \section plug_cppcode_PlugRegistry3 Python Code Example 3
/// \code
/// # Manufacture an instance of a derived type
/// imageFilterType = Tf.Type.Find(ImageFilter)
/// myFilterType = Tf.Type.FindByName('UltraCoolImageFilter')
/// if myFilterType and myFilterType.IsA(imageFilterType):
///     myFilter = myFilterType.Manufacture()
/// \endcode
///

class PLUG_API PlugRegistry : public TfWeakBase, boost::noncopyable {
public:
    typedef PlugRegistry This;
    typedef std::vector<TfType> TypeVector;

    /// Returns the singleton \c PlugRegistry instance.
    static PlugRegistry & GetInstance();

    /// Registers all plug-ins discovered at \a pathToPlugInfo.
    PlugPluginPtrVector RegisterPlugins(const std::string & pathToPlugInfo);

    /// Registers all plug-ins discovered in any of \a pathsToPlugInfo.
    PlugPluginPtrVector
    RegisterPlugins(const std::vector<std::string> & pathsToPlugInfo);

    /// Retrieve the \c TfType corresponding to the given \c name.  See the
    /// documentation for \c TfType::FindByName for more information.  Use this
    /// function if you expect that \c name may name a type provided by a
    /// plugin.  Calling this function will incur plugin discovery (but not
    /// loading) if plugin discovery has not yet occurred.
    static TfType FindTypeByName(std::string const &typeName);

    /// Retrieve the \c TfType that derives from \c base and has the given alias
    /// or type name \c typeName.  See the documentation for \c
    /// TfType::FindDerivedByName for more information.  Use this function if
    /// you expect that the derived type may be provided by a plugin.  Calling
    /// this function will incur plugin discovery (but not loading) if plugin
    /// discovery has not yet occurred.
    static TfType
    FindDerivedTypeByName(TfType base, std::string const &typeName);

    /// Retrieve the \c TfType that derives from \c Base and has the given alias
    /// or type name \c typeName.  See the documentation for \c
    /// TfType::FindDerivedByName for more information.  Use this function if
    /// you expect that the derived type may be provided by a plugin.  Calling
    /// this function will incur plugin discovery (but not loading) if plugin
    /// discovery has not yet occurred.
    template <class Base>
    static TfType
    FindDerivedTypeByName(std::string const &typeName) {
        return FindDerivedTypeByName(TfType::Find<Base>(), typeName);
    }

    /// Return a vector of types derived directly from \a base.  Use this
    /// function if you expect that plugins may provide types derived from \a
    /// base.  Otherwise, use \a TfType::GetDirectlyDerivedTypes.
    ///
    static std::vector<TfType>
    GetDirectlyDerivedTypes(TfType base);

    /// Return the set of all types derived (directly or indirectly) from 
    /// \a base.  Use this function if you expect that plugins may provide types
    /// derived from \a base.  Otherwise, use \a TfType::GetAllDerivedTypes.
    ///
    static void
    GetAllDerivedTypes(TfType base, std::set<TfType> *result);

    /// Return the set of all types derived (directly or indirectly) from 
    /// \a Base.  Use this function if you expect that plugins may provide types
    /// derived from \a base.  Otherwise, use \a TfType::GetAllDerivedTypes.
    ///
    template <class Base>
    static void
    GetAllDerivedTypes(std::set<TfType> *result) {
        return GetAllDerivedTypes(TfType::Find<Base>(), result);
    }

    /// Returns the plug-in for the given type, or a
    /// null pointer if the is no registered plug-in.
    PlugPluginPtr GetPluginForType(TfType t) const;

    /// Returns all registered plug-ins.
    PlugPluginPtrVector GetAllPlugins() const;

    /// Returns a plugin with the specified library name.
    PlugPluginPtr GetPluginWithName(const std::string& name) const;

    /// Looks for a string associated with \a type and \a key and returns it, or
    /// an empty string if \a type or \a key are not found.
    std::string GetStringFromPluginMetaData(TfType type, 
                                            const std::string &key) const;

    /// Looks for a JsValue associated with \a type and \a key and returns it,
    /// or a null JsValue if \a type or \a key are not found.
    JsValue GetDataFromPluginMetaData(TfType type,
                                      const std::string &key) const;

private:
    // Private ctor and dtor since this is a constructed as a singleton.
    PLUG_LOCAL
    PlugRegistry();

    // Registers all plug-ins discovered in any of \a pathsToPlugInfo
    // but does not send a notice.
    PLUG_LOCAL
    PlugPluginPtrVector
    _RegisterPlugins(const std::vector<std::string>& pathsToPlugInfo);

    template <class ConcurrentVector>
    PLUG_LOCAL
    void _RegisterPlugin(const Plug_RegistrationMetadata&,
                         ConcurrentVector *newPlugins);
    PLUG_LOCAL
    bool _InsertRegisteredPluginPath(const std::string &path);

    friend class TfSingleton< PlugRegistry >;
    friend class PlugPlugin; // For _RegisterPlugins().

private:
    TfHashSet<std::string, TfHash> _registeredPluginPaths;

    boost::scoped_ptr<class Plug_TaskArena> _dispatcher;

    std::mutex _mutex;
};

PLUG_API_TEMPLATE_CLASS(TfSingleton<PlugRegistry>);

#endif // PLUG_REGISTRY_H
