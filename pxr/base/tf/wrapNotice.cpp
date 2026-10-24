//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/tf/type.h"
#include "pxr/base/tf/notice.h"
#include "pxr/base/tf/pyFunction.h"
#include "pxr/base/tf/pyIdentity.h"
#include "pxr/base/tf/pyNoticeWrapper.h"
#include "pxr/base/tf/pyPtrHelpers.h"
#include "pxr/base/tf/pyWeakObject.h"
#include "pxr/base/tf/stringUtils.h"

#include <nanobind/nanobind.h>

#include <functional>

using std::string;

PXR_NAMESPACE_OPEN_SCOPE

class Tf_PyNotice {
public:
    static size_t SendWithType(const TfNotice& notice,
                               const TfType & noticeType,
                               const TfWeakBase* sender,
                               const void *senderUniqueId,
                               const std::type_info & senderType)
    {
        return notice._SendWithType(noticeType,
                                    sender, senderUniqueId, senderType);
    }
};

PXR_NAMESPACE_CLOSE_SCOPE

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

class Tf_PyNoticeInternal
{
  public:

    struct Listener : public TfWeakBase {

        typedef void CallbackSig(nanobind::object const &,
                                 nanobind::object const &);
        typedef std::function<CallbackSig> Callback;

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

        nanobind::object _GetDeliverableNotice(TfNotice const &notice,
                                               TfType const &noticeType) {
            // If the notice type is not wrapped, return the type name in a
            // string.
            TfPyLock lock;
            nanobind::object pyNotice = nanobind::find(notice);
            if (pyNotice.is_valid()) {
                return pyNotice;
            }
            pyNotice = Tf_PyNoticeObjectGenerator::Invoke(notice);
            if (pyNotice.is_valid()) {
                return pyNotice;
            }
            return nanobind::cast(TfType::Find(notice).GetTypeName());
        }
    
        void _HandleNotice(TfNotice const &notice,
                           TfType const &type,
                           TfWeakBase *sender,
                           void const *senderUniqueId,
                           const std::type_info &) {
            TfPyLock lock;
            nanobind::object pyNotice = _GetDeliverableNotice(notice, type);
            if (pyNotice.is_valid() && !pyNotice.is_none()) {
                // Get the python sender.
                nanobind::object pySender = nanobind::none();
                if (sender) {
                    if (PyObject *id = Tf_PyIdentityHelper::Get(senderUniqueId)) {
                        pySender = nanobind::steal(id);
                    }
                }
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
                             nanobind::handle sender) {
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

    static TfType
    _GetNoticeType(nanobind::handle self, TfNotice const &notice) {
        TfType noticeType = TfType::FindByPythonClass(
            TfPyObjWrapper(nanobind::borrow<nanobind::object>(self.type())));
        if (noticeType.IsUnknown()) {
            noticeType = TfType::Find(&notice);
        }
        return noticeType;
    }

    static size_t
    SendWithAnyWeakPtrSender(nanobind::handle self,
                             TfAnyWeakPtr const &sender) {
        TfNotice const &notice = nanobind::cast<TfNotice const &>(self);
        return Tf_PyNotice::SendWithType(notice, _GetNoticeType(self, notice),
                                         sender.GetWeakBase(),
                                         sender.GetUniqueIdentifier(),
                                         sender.GetTypeInfo());
    }

    static size_t
    SendWithPythonSender(nanobind::handle self, nanobind::handle sender) {
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
    
    static size_t SendGlobally(nanobind::handle self) {
        TfNotice const &notice = nanobind::cast<TfNotice const &>(self);
        return Tf_PyNotice::SendWithType(notice, _GetNoticeType(self, notice),
                                         0, 0, typeid(void));
    }

};

} // anonymous namespace 

void wrapNotice(nanobind::module_ &m)
{
    namespace nb = nanobind;

    // Passing TfNotice for both T and its base indicates that this is the root
    // of the notice hierarchy.
    auto notice = TfNbNoticeWrapper<TfNotice, TfNotice>::Wrap(m, "Notice");
    notice
        .def(nb::init<>())
        
        .def_static("Register",
             &Tf_PyNoticeInternal::RegisterWithAnyWeakPtrSender,
             nb::rv_policy::take_ownership)
        .def_static("Register",
             &Tf_PyNoticeInternal::RegisterWithPythonSender,
             nb::rv_policy::take_ownership,
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
        
        .def_static("RegisterGlobally",
             &Tf_PyNoticeInternal::RegisterGlobally,
             nb::rv_policy::take_ownership, 
             "RegisterGlobally( noticeType, callback ) -> Listener \n\n"
             "noticeType : Tf.Notice\n"
             "callback : function\n\n"
             "Register a listener as being interested in a TfNotice "
             "type from any sender.  The notice listener does not get sender "
             "as an argument. "
             "")

        .def("Send", &Tf_PyNoticeInternal::SendWithAnyWeakPtrSender)
        .def("Send", &Tf_PyNoticeInternal::SendWithPythonSender,
             "Send(sender) \n\n"
             "sender : object \n\n"
             "Deliver the notice to interested listeners, returning the number "
             "of interested listeners. "
             "This is the recommended form of Send.  It takes the sender as an "
             "argument. "
             "Listeners that registered for the given sender AND listeners "
             "that registered globally will get the notice. ")
        .def("SendGlobally", &Tf_PyNoticeInternal::SendGlobally,
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
    nb::class_<Tf_PyNoticeInternal::Listener>(notice, "Listener", Listener_string)
        .def("Revoke", &Tf_PyNoticeInternal::Listener::Revoke,
            "Revoke() \n\n"
            "Revoke interest by a notice listener. "
            " "
            "This function revokes interest in the "
            "particular notice type and call-back method that its "
            "Listener object was registered for. "
        )
        ;
}
