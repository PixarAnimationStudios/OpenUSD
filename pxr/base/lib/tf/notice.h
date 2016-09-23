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
#ifndef TF_NOTICE_H
#define TF_NOTICE_H

/// \file tf/notice.h
/// \ingroup group_tf_Notification

#include "pxr/base/tf/anyWeakPtr.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/type.h"
#include "pxr/base/tf/weakPtr.h"

#include "pxr/base/arch/demangle.h"
#include "pxr/base/arch/hints.h"

#include <list>
#include <typeinfo>

class Tf_NoticeRegistry;

/// \class TfNotice 
/// \ingroup group_tf_Notification
///
/// The base class for objects used to notify interested parties (listeners)
/// when events have occured.  The TfNotice class also serves as a container
/// for various dispatching routines such as Register() and Send().
///
/// See \ref page_tf_Notification in the C++ API reference for a detailed 
/// description of the notification system.
///
/// \section pycode_TfNotice Python Example: Registering For and Sending
/// Notices The following code provides examples of how to set up a Notice
/// listener connection (represented in Python by the Listener class),
/// including creating and sending notices, registering to receive notices,
/// and breaking a listener connection.
/// \code{.py}
/// # To create a new notice type:
/// class APythonClass(Tf.Notice):
///     '''TfNotice sent when APythonClass does something of interest.'''
///     pass
/// Tf.Type.Define(APythonClass)
/// 
/// # An interested listener can register to receive notices from all
/// # senders, or from a particular type of sender.
///
/// # To send a notice to all registered listeners:;
/// APythonClass().SendGlobally()
///
/// # To send a notice to listeners who register with a specific sender:
/// APythonClass().Send(self)
///
/// # To register for the notice from any sender:
/// my_listener = Tf.Notice.RegisterGlobally(APythonClass, self._HandleNotice)
///
/// # To register for the notice from a specific sender
/// my_listener = Tf.Notice.Register(APythonClass, self._HandleNotice, sender)
/// 
/// def _HandleNotice(self, notice, sender):
///    '''callback function for handling a notice'''
///    # do something when the notice arrives
///
/// # To revoke interest in a notice
/// my_listener.Revoke()
/// \endcode
/// 
/// For more on using notices in Python, see the Editor With Notices tutorial.
/// 
class TfNotice {
private:
    class _DelivererBase;
    typedef TfWeakPtr<_DelivererBase> _DelivererWeakPtr;
    typedef std::list<_DelivererBase*> _DelivererList;
    
    ////////////////////////////////////////////////////////////////////////
    // Per-sender delivery, listener gets sender.
    template <class LPtr, class L,
              class Notice, class SPtr, class DeliveredSPtr>
    static _DelivererBase *
    _MakeDeliverer(LPtr const &listener, 
                   void (L::*method)
                   (const Notice &, DeliveredSPtr const &),
                   SPtr const &sender) {
        DeliveredSPtr weakSender(sender);
        return new _DelivererWithSender<
            LPtr, DeliveredSPtr,
            void (L::*)(const Notice &, DeliveredSPtr const &),
            Notice
        >(listener, method, weakSender);
    }

    template <class LPtr, class L,
              class Notice, class SPtr, class DeliveredSPtr>
    static _DelivererBase *
    _MakeDeliverer(LPtr const &listener, 
                   void (L::*method)
                   (const Notice &, DeliveredSPtr const &) const,
                   SPtr const &sender) {
        DeliveredSPtr weakSender(sender);
        return new _DelivererWithSender<
            LPtr, DeliveredSPtr,
            void (L::*)(const Notice &, DeliveredSPtr const &) const,
            Notice
        >(listener, method, weakSender);
    }

    ////////////////////////////////////////////////////////////////////////
    // Per-sender delivery, listener does not get sender.
    template <class LPtr, class L, class SPtr, class Notice>
    static _DelivererBase *
    _MakeDeliverer(LPtr const &listener,
                   void (L::*method)(const Notice &),
                   SPtr const &sender) {
        return new _Deliverer<
            LPtr, SPtr, void (L::*)(const Notice &), Notice
        >(listener, method, sender);
    }

    template <class LPtr, class L, class SPtr, class Notice>
    static _DelivererBase *
    _MakeDeliverer(LPtr const &listener,
                   void (L::*method)(const Notice &) const,
                   SPtr const &sender) {
        return new _Deliverer<
            LPtr, SPtr, void (L::*)(const Notice &) const, Notice
        >(listener, method, sender);
    }

    ////////////////////////////////////////////////////////////////////////
    // Global delivery.
    template <class LPtr, class L, class Notice>
    static _DelivererBase *
    _MakeDeliverer(LPtr const &listener,
                   void (L::*method)(const Notice &)) {
        return new _Deliverer<
            LPtr, TfAnyWeakPtr, void (L::*)(const Notice &), Notice
        >(listener, method);
    }

    template <class LPtr, class L, class Notice>
    static _DelivererBase *
    _MakeDeliverer(LPtr const &listener,
                   void (L::*method)(const Notice &) const) {
        return new _Deliverer<
            LPtr, TfAnyWeakPtr, void (L::*)(const Notice &) const, Notice
        >(listener, method);
    }

    ////////////////////////////////////////////////////////////////////////
    // Generic (raw) delivery.
    template <class LPtr, class L>
    static _DelivererBase *
    _MakeDeliverer(TfType const &noticeType,
                   LPtr const &listener,
                   void (L::*method)(const TfNotice &,
                                     const TfType &,
                                     TfWeakBase*, const void *,
                                     const std::type_info&),
                   TfAnyWeakPtr const &sender) {
        return new _RawDeliverer<LPtr,
            void (L::*)(const TfNotice &, const TfType &,
                        TfWeakBase *, const void *,
                        const std::type_info &)>
            (listener, method, sender, noticeType);
    }

    template <class LPtr, class L>
    static _DelivererBase *
    _MakeDeliverer(TfType const &noticeType,
                   LPtr const &listener,
                   void (L::*method)(const TfNotice &,
                                     const TfType &,
                                     TfWeakBase*, const void *,
                                     const std::type_info&) const,
                   TfAnyWeakPtr const &sender)
    {
        return new _RawDeliverer<LPtr,
            void (L::*)(const TfNotice &, const TfType &,
                        TfWeakBase *, const void *,
                        const std::type_info &) const>
            (listener, method, sender, noticeType);
    }

    
    
public:

    class Probe;
    typedef TfWeakPtr<Probe> WeakProbePtr;
    
    /// Probe interface class which may be implemented and then registered via
    /// \c InsertProbe to introspect about notices as they are sent and
    /// delivered.
    class Probe : public TfWeakBase {
      public:
        virtual ~Probe() = 0;

        /// This method is called just before \p notice is sent to any
        /// listeners.  \p sender is NULL if \p notice is sent globally.  In
        /// this case, \p senderType will be typeid(void).
        virtual void BeginSend(const TfNotice &notice,
                               const TfWeakBase *sender,
                               const std::type_info &senderType) = 0;

        /// This method is called after the notice in the corresponding \c
        /// BeginSend call has been delivered to all listeners.
        virtual void EndSend() = 0;

        /// This method is called just before \p notice is
        /// delivered to a listener.  \p sender is NULL if \p notice is
        /// sent globally or the listener is global.  In this case, \p
        /// senderType will be typeid(void).
        virtual void BeginDelivery(const TfNotice &notice,
                                   const TfWeakBase *sender,
                                   const std::type_info &senderType,
                                   const TfWeakBase *listener,
                                   const std::type_info &listenerType) = 0;

        /// This method is called after the notice in the
        /// corresponding \c BeginDelivery call has finished being
        /// processed by its listener.
        virtual void EndDelivery() = 0;
    };

    /// Handle-object returned by \c TfNotice::Register().
    ///
    /// When a listener is registered by \c TfNotice::Register(), an object of
    /// type \c TfNotice::Key is returned; this key object can be given to \c
    /// Revoke() to subequently unregister the listener with respect to that
    /// particular notice type and callback method.
    class Key {
    public:
        Key() {}

        /// Does this key refer to a valid notification?
        ///
        /// \c IsValid will return true if this key refers to a currently
        /// active notification.  Revoking the key will make it invalid again.
        bool IsValid() const {
            return _deliverer and _deliverer->_IsActive();
        }

        /// Does this key refer to a valid notification?
        ///
        /// The boolean operator is identical to \c IsValid() above.
        operator bool() const {
            return IsValid();
        }

    private:
        Key(const _DelivererWeakPtr & d) : _deliverer(d) {}

        _DelivererWeakPtr _deliverer;

        friend class Tf_NoticeRegistry;
        friend class TfNotice;
    };

    /// A \c TfNotice::Key container.
    ///
    /// Many listeners listen for several notices and must revoke interest for
    /// those several notices at once.  These listeners can put all of the
    /// keys into a \c TfNotice::Keys then call \c Revoke() on it.
    typedef std::vector<Key> Keys;

    /// Register a probe that will be invoked when notices are sent and
    /// delivered.  
    /// \see TfNotice::Probe
    static void InsertProbe(const WeakProbePtr &probe);

    /// Remove a probe that was previously registered with \c InsertProbe.
    /// \see TfNotice::Probe
    static void RemoveProbe(const WeakProbePtr &probe);

    /// Register a listener as being interested in a \c TfNotice.
    ///
    /// Registration of interest in a notice class \c N automatically
    /// registers interest in all classes derived from \c N.  When a notice of
    /// appropriate type is received, the listening object's member-function
    /// \p method is called with the notice.
    ///
    /// Supports several forms of registration.
    ///
    /// - Listening for a notice from a particular sender.
    ///
    /// \code
    /// // Listener does not receive sender.
    /// void Listener::_HandleNotice(SomeNotice const &notice) [const];
    /// Register(listenerPtr, &Listener::_HandleNotice, senderPtr);
    ///
    /// // Listener receives sender.
    /// void Listener::_HandleNoticeSender(SomeNotice const &notice,
    ///                                    SenderPtr const &sender) [const];
    /// Register(listenerPtr, &Listener::_HandleNoticeSender, senderPtr);
    /// \endcode
    ///
    /// - Listening for a notice globally.  Prefer listening to a notice from a
    /// particular sender whenever possible (as above).
    ///
    /// \code
    /// void Listener::_HandleGlobalNotice(SomeNotice const &notice) [const];
    /// Register(listenerPtr, &Listener::_HandleGlobalNotice);
    /// \endcode
    ///
    /// - Listening for a notice dynamically, with a type that is unknown at
    /// compile-time.  This facility is used for some internal mechanisms,
    /// such as bridging notice delivery into Python, and is not meant for
    /// public consumption.
    ///
    /// \code
    /// void Listener::_HandleGenericNotice(TfNotice const &notice,
    ///                                     TfType const &noticeType,
    ///                                     TfWeakBase *sender,
    ///                                     void const *senderUniqueId,
    ///                                     std::type_info const &senderType)
    ///                                     [const];
    /// Register(listenerPtr,
    ///          &Listener::_HandleGenericNotice, noticeType, senderPtr);
    /// \endcode
    /// 
    /// The listener being registered must be pointed to by a \c
    /// TfWeakPtrFacade, like a TfWeakPtr or another TfWeakPtrFacade-based
    /// Handle.  The sender being registered for (if any) must also be pointed
    /// to by a \c TfWeakPtrFacade.
    ///
    /// Note that the notification center only holds onto the listening object
    /// via a \c TfWeakPtr.  That is, it does not influence the lifetime of
    /// that object.
    ///
    /// To reverse the registration, call \c Key::Revoke() on the \c Key
    /// object returned by this call.
    template <class LPtr, class MethodPtr>
    static TfNotice::Key
    Register(LPtr const &listener, MethodPtr method) {
        return _Register(_MakeDeliverer(listener, method));
    }

    template <class LPtr, class MethodPtr, class SenderPtr>
    static TfNotice::Key
    Register(LPtr const &listener, MethodPtr method, SenderPtr const &sender) {
        return _Register(_MakeDeliverer(listener, method, sender));
    }

    template <class LPtr, class MethodPtr>
    static TfNotice::Key
    Register(LPtr const &listener, MethodPtr method,
             const TfType &noticeType, const TfAnyWeakPtr &sender) {
        return _Register(_MakeDeliverer(noticeType, listener, method, sender));
    }
    
    /// Revoke interest by a listener.
    ///
    /// This revokes interest by the listener for the particular notice type
    /// and call-back method for which this key was created.
    ///
    /// \c Revoke will return a bool value indicating whether or not the key
    /// was successfully revoked.  Subsequent calls to \c Revoke with the same
    /// key will return false.
    static bool Revoke(TfNotice::Key& key);
    
    /// Revoke interest by listeners.
    ///
    /// This revokes interest by the listeners for the particular
    /// notice types and call-back methods for which the keys were
    /// created.  It then clears the keys container.
    static void Revoke(TfNotice::Keys* keys);

    /// Deliver the notice to interested listeners, returning the number
    /// of interested listeners.  
    ///
    /// For most clients it is recommended to use the Send(sender) version of
    /// Send() rather than this one.  Clients that use this form of Send
    /// will prevent listeners from being able to register to receive notices
    /// based on the sender of the notice.
    ///
    /// ONLY listeners that registered globally will get the notice.
    ///
    /// Listeners are invoked synchronously and in arbitrary order. The value
    /// returned is the total number of times the notice was sent to listeners.
    /// Note that a listener is called in the thread in which \c Send() is called
    /// and \e not necessarily in the thread that \c Register() was called in.
    size_t Send() const;
    
    /// Deliver the notice to interested listeners, returning the number of
    /// interested listeners.
    ///
    /// This is the recommended form of Send.  It takes the sender as an
    /// argument.
    ///
    /// Listeners that registered for the given sender AND listeners that
    /// registered globally will get the notice.
    ///
    /// Listeners are invoked synchronously and in arbitrary order. The value
    /// returned is the total number of times the notice was sent to
    /// listeners. Note that a listener is called in the thread in which \c
    /// Send() is called and \e not necessarily in the thread that \c
    /// Register() was called in.
    template <typename SenderPtr>
    size_t Send(SenderPtr const &s) const;
    
    /// Variant of Send() that takes a specific sender in the form of a
    /// TfWeakBase pointer and a typeid.
    ///
    /// This version is used by senders who don't have static knowledge of
    /// sender's type, but have access to its weak base pointer and its
    /// typeid.
    size_t SendWithWeakBase(const TfWeakBase *senderWeakBase,
                            const void *senderUniqueId,
                            const std::type_info &type) const;

    virtual ~TfNotice();

    /// Blocks sending of all notices in current thread.
    ///
    /// \note This is intended to be temporary and should NOT be used.
    ///
    /// While one or more \c TfNotice::Block is instantiated, any call to \c
    /// TfNotice::Send in the current thread will be silently ignored.  This
    /// will continue until all \c TfNotice::Block objects are destroyed.
    /// Notices that are sent when blocking is active will *not* be resent.
    class Block {
    public:
        Block();
        ~Block();
    };

private:
    // Abstract base class for calling listeners.
    // A typed-version derives (via templating) off this class.
    class _DelivererBase : public TfWeakBase {
    public:
        _DelivererBase()
            : _list(0), _active(true), _markedForRemoval(false)
        {
        }
        
        virtual ~_DelivererBase();

        void _BeginDelivery(const TfNotice &notice,
                            const TfWeakBase *sender,
                            const std::type_info &senderType,
                            const TfWeakBase *listener,
                            const std::type_info &listenerType,
                            const std::vector<TfNotice::WeakProbePtr> &probes);
        void _EndDelivery(const std::vector<TfNotice::WeakProbePtr> &probes);

        // The derived class converts n to the proper type and delivers it by
        // calling the listener's method.  The function returns \c true,
        // unless the listener has expired or been marked in active (i.e. by
        // TfNotice::Revoke()), in which case the method call is skipped and
        // \c false is returned.
        virtual bool
        _SendToListener(const TfNotice &n,
                        const TfType &type,
                        const TfWeakBase *s,
                        const void *senderUniqueId,
                        const std::type_info &senderType,
                        const std::vector<TfNotice::WeakProbePtr> &probes) = 0;

        void _Deactivate() {
            _active = false;
        }
        
        bool _IsActive() const {
            return _active;
        }

        void _MarkForRemoval() {
            _markedForRemoval = true;
        }
        
        // True if the entry has been added to the _deadEntries list for
        // removal.  Used to avoid adding it more than once to the list.
        bool _IsMarkedForRemoval() const {
            return _markedForRemoval;
        }

        virtual TfType GetNoticeType() const = 0;
        
        virtual bool Delivers(TfType const &noticeType,
                              const TfWeakBase *sender) const = 0;
        
        virtual TfWeakBase const *GetSenderWeakBase() const = 0;

        virtual _DelivererBase *Clone() const = 0;
        
    protected:
        
        template <class ToNoticeType, class FromNoticeType>
        static inline ToNoticeType const *
        _CastNotice(FromNoticeType const *from) {
            // Dynamic casting in deliverers is significant overhead, so only
            // do error checking in debug builds.
            if (TF_DEV_BUILD) {
                if (!dynamic_cast<ToNoticeType const *>(from)) {
                    ToNoticeType const *castNotice =
                        TfSafeDynamic_cast<ToNoticeType const *>(from);
                    // this will abort with a clear error message if
                    // castNotice is NULL
                    TfNotice::_VerifyFailedCast(typeid(ToNoticeType),
                                                *from, castNotice);
                }
            }
            return static_cast<ToNoticeType const *>(from);
        }
        
    private:
        // Linkage to the containing _DelivererList in the Tf_NoticeRegistry
        _DelivererList *_list;
        _DelivererList::iterator _listIter;
        
        bool _active;
        bool _markedForRemoval;
        
        friend class Tf_NoticeRegistry;
    };

    template <class Derived>
    class _StandardDeliverer : public _DelivererBase {
    public:
        virtual ~_StandardDeliverer() {}

        virtual TfType GetNoticeType() const {
            typedef typename Derived::NoticeType NoticeType;
            TfType ret = TfType::Find<NoticeType>();
            if (ret.IsUnknown())
                TF_FATAL_ERROR("notice type " + ArchGetDemangled<NoticeType>() +
                               " undefined in the TfType system");
            return ret;
        }

        virtual bool Delivers(TfType const &noticeType,
                              TfWeakBase const *sender) const {
            Derived const *derived = this->AsDerived();
            return noticeType.IsA(GetNoticeType()) and
                not derived->_sender.IsInvalid() and
                sender and derived->_sender.GetWeakBase() == sender;
        }

        virtual TfWeakBase const *GetSenderWeakBase() const {
            Derived const *derived = this->AsDerived();
            return derived->_sender ? derived->_sender.GetWeakBase() : 0;
        }

        virtual _DelivererBase *Clone() const {
            Derived const *derived = this->AsDerived();
            return new Derived(derived->_listener,
                               derived->_method,
                               derived->_sender,
                               GetNoticeType());
        }
        
        virtual bool
        _SendToListener(const TfNotice &notice,
                        const TfType &noticeType,
                        const TfWeakBase *sender,
                        const void *senderUniqueId,
                        const std::type_info &senderType,
                        const std::vector<TfNotice::WeakProbePtr> &probes)
        {
            Derived *derived = this->AsDerived();
            typedef typename Derived::ListenerType ListenerType;
            typedef typename Derived::NoticeType NoticeType;
            ListenerType *listener = get_pointer(derived->_listener);

            if (listener and not derived->_sender.IsInvalid()) {
                if (ARCH_UNLIKELY(not probes.empty())) {
                    TfWeakBase const *senderWeakBase = GetSenderWeakBase(),
                        *listenerWeakBase = derived->_listener.GetWeakBase();
                    _BeginDelivery(notice, senderWeakBase,
                                   senderWeakBase ?
                                   senderType : typeid(void),
                                   listenerWeakBase,
                                   typeid(ListenerType), probes);
                }

                derived->
                    _InvokeListenerMethod(listener,
                                          *_CastNotice<NoticeType>(&notice),
                                          noticeType, sender,
                                          senderUniqueId, senderType);
                
                if (ARCH_UNLIKELY(not probes.empty()))
                    _EndDelivery(probes);

                return true;
            }
            return false;
        }
        
    private:
        Derived *AsDerived() {
            return static_cast<Derived *>(this);
        }

        Derived const *AsDerived() const {
            return static_cast<Derived const *>(this);
        }
    };
    
    
    template <typename LPtr, typename SPtr, typename Method, typename Notice>
    class _Deliverer :
        public _StandardDeliverer<_Deliverer<LPtr, SPtr, Method, Notice> >
    {
    public:
        typedef Notice NoticeType;
        typedef typename LPtr::DataType ListenerType;
        typedef Method MethodPtr;
        
        _Deliverer(LPtr const &listener,
                   MethodPtr const &methodPtr,
                   SPtr const &sender = SPtr(),
                   TfType const &noticeType = TfType())
            : _listener(listener)
            , _sender(sender)
            , _method(methodPtr)
        {
        }

        void _InvokeListenerMethod(ListenerType *listener,
                                   const NoticeType &notice,
                                   const TfType &noticeType,
                                   const TfWeakBase *,
                                   const void *,
                                   const std::type_info &)
        {
            (listener->*_method)(notice);
        }
        
        LPtr _listener;
        SPtr _sender;
        MethodPtr _method;
    };
    
    template <class LPtr, class Method>
    class _RawDeliverer :
        public _StandardDeliverer<_RawDeliverer<LPtr, Method> >
    {
    public:
        typedef TfNotice NoticeType;
        typedef typename LPtr::DataType ListenerType;
        typedef Method MethodPtr;

        _RawDeliverer(LPtr const &listener,
                      MethodPtr const &methodPtr,
                      TfAnyWeakPtr const &sender,
                      TfType const &noticeType)
            : _noticeType(noticeType),
              _listener(listener),
              _method(methodPtr),
              _sender(sender)
        {
        }

        virtual TfType GetNoticeType() const {
            return _noticeType;
        }

        void _InvokeListenerMethod(ListenerType *listener,
                                   const NoticeType &notice,
                                   const TfType &noticeType,
                                   const TfWeakBase *sender,
                                   const void *senderUniqueId,
                                   const std::type_info &senderType)
        {
            (listener->*_method)(notice, noticeType,
                                 const_cast<TfWeakBase *>(sender),
                                 senderUniqueId, senderType);
        }
        
        TfType _noticeType;
        LPtr _listener;
        MethodPtr _method;
        TfAnyWeakPtr _sender;
    };

    template <class LPtr, class SPtr, class Method, class Notice>
    class _DelivererWithSender :
        public _StandardDeliverer<
            _DelivererWithSender<LPtr, SPtr, Method, Notice>
        >
    {
    public:
        typedef Notice NoticeType;
        typedef Method MethodPtr;
        typedef typename LPtr::DataType ListenerType;

        typedef typename SPtr::DataType SenderType;

        _DelivererWithSender(LPtr const &listener,
                             MethodPtr const &methodPtr,
                             SPtr const &sender,
                             TfType const &noticeType = TfType())
            : _listener(listener),
              _sender(sender),
              _method(methodPtr)
        {
        }
 
        void _InvokeListenerMethod(ListenerType *listener,
                                   const NoticeType &notice,
                                   const TfType &noticeType,
                                   const TfWeakBase *sender,
                                   const void *,
                                   const std::type_info &)
        {
            SenderType *deliveredSender =
                static_cast<SenderType *>(const_cast<TfWeakBase *>(sender));
            SPtr deliveredSPtr(deliveredSender);
            (listener->*_method)(notice, deliveredSPtr);
        }
        
        LPtr _listener;
        SPtr _sender;
        MethodPtr _method;
    };

private:
    // Internal non-templated function to install listeners.
    static Key _Register(_DelivererBase*);

    static void _VerifyFailedCast(const std::type_info& toType,
                                  const TfNotice& notice, const TfNotice* castNotice);

    size_t _Send(const TfWeakBase* sender,
                 const void *senderUniqueId,
                 const std::type_info & senderType) const;
    size_t _SendWithType(const TfType & noticeType,
                         const TfWeakBase* sender,
                         const void *senderUniqueId,
                         const std::type_info & senderType) const;

    friend class Tf_NoticeRegistry;

    // Befriend the wrapping so it can access _SendWithType() directly
    // in order to provide dynamic downcasting of Python notice types.
    friend class Tf_PyNotice;
};

template <typename SenderPtr> 
size_t 
TfNotice::Send(SenderPtr const &s) const
{
    const TfWeakBase *senderWeakBase = s ? s.GetWeakBase() : NULL;
    return _Send(senderWeakBase, senderWeakBase ? s.GetUniqueIdentifier() : 0,
                 senderWeakBase ?
                 typeid(typename SenderPtr::DataType) : typeid(void));
}

#endif
