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
#include "pxr/base/tf/type.h"
#include "pxr/base/tf/notice.h"
#include "pxr/base/tf/pyFunction.h"
#include "pxr/base/tf/pyIdentity.h"
#include "pxr/base/tf/pyNoticeWrapper.h"
#include "pxr/base/tf/pyPtrHelpers.h"
#include "pxr/base/tf/pyWeakObject.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/tf/type.h"

#include <boost/function.hpp>
#include <boost/noncopyable.hpp>
#include <boost/python/class.hpp>
#include <boost/python/def.hpp>
#include <boost/python/handle.hpp>
#include <boost/python/manage_new_object.hpp>
#include <boost/python/return_value_policy.hpp>
#include <boost/python/scope.hpp>


using std::string;

using namespace boost::python;

class Tf_PyNotice
{
  public:

    struct Listener : public TfWeakBase, public boost::noncopyable {

        typedef void CallbackSig(object const &, handle<> const &);
        typedef boost::function<CallbackSig> Callback;

        static Listener *New(TfType const &noticeType,
                             Callback const &callback,
                             TfAnyWeakPtr const &sender)
        {
            if (noticeType.IsA<TfNotice>()) 
                return new Listener(noticeType, callback, sender);

            // Fail -- unknown notice type.
            TfPyThrowTypeError("not registering for '" +
                               noticeType.GetTypeName() +
                               "' because it is not a known TfNotice type");
            // CODE_COVERAGE_OFF The above throws, so this is unreachable.
            return 0;
            // CODE_COVERAGE_ON
        }

        ~Listener() { Revoke(); }
        void Revoke() { TfNotice::Revoke(_key); }

      private:
    
        Listener(TfType const &noticeType,
                 Callback const &callback,
                 TfAnyWeakPtr const &sender) :
            _callback(callback), _noticeType(noticeType) {
            
            _key = TfNotice::Register
                (TfCreateWeakPtr(this), &Listener::_HandleNotice,
                 noticeType, sender);
        }

        object _GetDeliverableNotice(TfNotice const &notice,
                                     TfType const &noticeType) {
            // If the notice type is not wrapped, return the type name in a
            // string.
            TfPyLock lock;
            /// XXX noticeType is incorrect when the notice is
            /// python-implemented.  We should fix this when TfType optimization
            /// work is done.
            object noticeClass = TfPyGetClassObject(typeid(notice));
            if (TfPyIsNone(noticeClass))
                return object(TfType::Find(notice).GetTypeName());

            // If it's a python notice, use the embedded python object.
            if (TfPyNoticeWrapperBase const *pyNotice =
                TfSafeDynamic_cast<TfPyNoticeWrapperBase const *>(&notice))
                return object(pyNotice->GetNoticePythonObject());

            // Otherwise convert the notice to python like normal.  We
            // can't just use object(notice) because that won't produce
            // a notice of the correct derived type.
            return Tf_PyNoticeObjectGenerator::Invoke(notice);
        }
    
        void _HandleNotice(TfNotice const &notice,
                           TfType const &type,
                           TfWeakBase *sender,
                           void const *senderUniqueId,
                           const std::type_info &) {
            TfPyLock lock;
            object pyNotice = _GetDeliverableNotice(notice, type);
            if (not TfPyIsNone(pyNotice)) {
                // Get the python sender.
                handle<> pySender = sender ?
                    handle<>(allow_null(Tf_PyIdentityHelper::
                                        Get(senderUniqueId))) : handle<>();
                _callback(pyNotice, pySender);
            }
        }

        Callback _callback;
        TfNotice::Key _key;
        TfType _noticeType;
    };

    static Listener *
    RegisterWithAnyWeakPtrSender(TfType const &noticeType,
                                 Listener::Callback const &cb,
                                 TfAnyWeakPtr const &sender)
    {
        return Listener::New(noticeType, cb, sender);
    }

    static Listener *
    RegisterWithPythonSender(TfType const &noticeType,
                             Listener::Callback const &cb,
                             object const &sender) {
        Tf_PyWeakObjectPtr weakSender = Tf_PyWeakObject::GetOrCreate(sender);
        if (!weakSender)
            TfPyThrowTypeError("Cannot register to listen to notices from the "
                               "provided sender.  The sender must support "
                               "python weak references.");

        TfAnyWeakPtr holder(weakSender);
        return RegisterWithAnyWeakPtrSender(noticeType, cb, holder);
    }
    
    static Listener *
    RegisterGlobally(TfType const &noticeType, Listener::Callback const &cb) {
        return RegisterWithAnyWeakPtrSender
            (noticeType, cb, TfAnyWeakPtr());
    }

    static size_t
    SendWithAnyWeakPtrSender(TfNotice const &self,
                             TfAnyWeakPtr const &sender) {
        return self._SendWithType(TfType::Find(&self),
                                  sender.GetWeakBase(),
                                  sender.GetUniqueIdentifier(),
                                  sender.GetTypeInfo());
    }

    static size_t
    SendWithPythonSender(TfNotice const &self, object const &sender) {
        // Get a "WeakObjectPtr" corresponding to the sender -- this is a
        // TfWeakPtr to an object which holds a python weak reference.  This
        // object expires when the python object expires.  This is what lets us
        // use arbitrary python objects as senders in the notice system.
        Tf_PyWeakObjectPtr weakSender = Tf_PyWeakObject::GetOrCreate(sender);
        if (!weakSender)
            TfPyThrowTypeError("Cannot send notice from the provided sender.  "
                               "Sender must support python weak references.");
        TfAnyWeakPtr holder(weakSender);
        return SendWithAnyWeakPtrSender(self, holder);
    }
    
    static size_t SendGlobally(TfNotice const &self) {
        return self._SendWithType(TfType::Find(&self),
                                  0, 0, typeid(void));
    }

};


// TfNotice is passed for both the type and the base to indicate the root of the
// hierarchy.
TF_INSTANTIATE_NOTICE_WRAPPER(TfNotice, TfNotice);

void wrapNotice()
{
    // Make sure we can pass callbacks from python.
    TfPyFunctionFromPython<Tf_PyNotice::Listener::CallbackSig>();

    // Passing TfNotice for both T and its base indicates that this is the root
    // of the notice hierarchy.
    scope notice = TfPyNoticeWrapper<TfNotice, TfNotice>::Wrap("Notice")
        .def(init<>())
        
        // We register the method that takes any python object first, as this is
        // the last overload that will be tried.  Thus, it will only be invoked
        // if the python object is not already weak-pointable.
        .def("Register",
             Tf_PyNotice::RegisterWithPythonSender,
             return_value_policy<manage_new_object>(),
         "Register( noticeType, callback, sender ) -> Listener \n\n"
         "noticeType : Tf.Notice\n"
         "callback : function\n"
         "sender : object\n\n"
         "Register a listener as being interested in a TfNotice  "
         "type from a specific sender.  Notice listener will get sender  "
         "as an argument.   "
         "  "
         "Registration of interest in a notice class N automatically  "
         "registers interest in all classes derived from N.  When a  "
         "notice of appropriate type is received, the listening object's "
         " member-function method is called with the notice. "
         "  "
         "  "
         "To reverse the registration, call Revoke() on the Listener "
         "object returned by this call. "
             )
        .def("Register",
             Tf_PyNotice::RegisterWithAnyWeakPtrSender,
             return_value_policy<manage_new_object>())
        .staticmethod("Register")
        
        .def("RegisterGlobally",
             Tf_PyNotice::RegisterGlobally,
             return_value_policy<manage_new_object>(), 
             "RegisterGlobally( noticeType, callback ) -> Listener \n\n"
             "noticeType : Tf.Notice\n"
             "callback : function\n\n"
             "Register a listener as being interested in a TfNotice "
             "type from any sender.  The notice listener does not get sender "
             "as an argument. "
             "")
        .staticmethod("RegisterGlobally")


        // We register the method that takes any python object first, as this is
        // the last overload that will be tried.  Thus, it will only be invoked
        // if the python object is not already weak-pointable.
        .def("Send", &Tf_PyNotice::SendWithPythonSender,
             "Send(sender) \n\n"
             "sender : object \n\n"
             "Deliver the notice to interested listeners, returning the number "
             "of interested listeners. "
             "This is the recommended form of Send.  It takes the sender as an "
             "argument. "
             "Listeners that registered for the given sender AND listeners "
             "that registered globally will get the notice. ")
        .def("Send", &Tf_PyNotice::SendWithAnyWeakPtrSender)
        .def("SendGlobally", &Tf_PyNotice::SendGlobally,
             "SendGlobally() \n\n"
             "Deliver the notice to interested listeners.   "
             "For most clients it is recommended to use the Send(sender) "
             "version of "
             "Send() rather than this one.  Clients that use this form of Send "
             "will prevent listeners from being able to register to receive "
             "notices "
             "based on the sender of the notice. "
             "ONLY listeners that registered globally will get the notice. ") 
        ;

    const char* Listener_string = ""
    "Represents the Notice connection between "
    "senders and receivers of notices.  When a Listener object expires the "
    "connection is broken. "
    "You can also use the Revoke() function to break the connection. "
    "A Listener object is returned from the Register() and  "
    "RegisterGlobally() functions. ";
    class_<Tf_PyNotice::Listener, boost::noncopyable>("Listener", Listener_string, no_init)
        .def("Revoke", &Tf_PyNotice::Listener::Revoke,
            "Revoke() \n\n"
            "Revoke interest by a notice listener. "
            " "
            "This function revokes interest in the "
            "particular notice type and call-back method that its "
            "Listener object was registered for. "
        )
        ;
}


