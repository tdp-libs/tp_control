#ifndef tp_control_CoreInterface_h
#define tp_control_CoreInterface_h

#include "tp_control/Globals.h"

#include "tp_utils/StringID.h"

#include "json.hpp"

#include <functional>
#include <unordered_map>

namespace tp_control
{

class CoreInterface;
class CoreInterfaceData;
struct CoreInterfacePayloadPrivate;

//##################################################################################################
//! The callback for changes in the list of channels.
typedef std::function<void()> ChannelListChangedCallback;

//##################################################################################################
//! The callback for changes to channel data.
typedef std::function<void(const tp_utils::StringID& typeID, const tp_utils::StringID& nameID, const CoreInterfaceData* data)> ChannelChangedCallback;

//##################################################################################################
//! The callback for signals.
typedef std::function<void(const tp_utils::StringID& typeID, const CoreInterfaceData* data)> SignalCallback;

//##################################################################################################
//! The payload for signals and channels
class TP_CONTROL_SHARED_EXPORT CoreInterfaceData
{
public:
  //################################################################################################
  virtual ~CoreInterfaceData()=default;
};

//##################################################################################################
//! A handle to a channel in a core interface
class TP_CONTROL_SHARED_EXPORT CoreInterfaceHandle
{
  friend class CoreInterface;
  CoreInterfacePayloadPrivate* m_payload{nullptr};
  tp_utils::StringID m_typeID;
  tp_utils::StringID m_nameID;

  //################################################################################################
  CoreInterfaceHandle(const tp_utils::StringID& typeID_, const tp_utils::StringID& nameID_);

public:
  //################################################################################################
  CoreInterfaceHandle()=default;

  //################################################################################################
  CoreInterfaceHandle(const CoreInterfaceHandle& other)=default;

  //################################################################################################
  const tp_utils::StringID& typeID() const;

  //################################################################################################
  const tp_utils::StringID& nameID() const;

  //################################################################################################
  //! Compare handles
  /*!
  Returns true if both handles are valid and pointing to the same channel in the same interface.

  \param other - The handle to compare with this.
  \return True if the handles are valid and match.
  */
  bool operator==(const CoreInterfaceHandle& other)const;

  //################################################################################################
  //! Returns the channel data
  /*!
  This returns the data that this channel holds, or an inalid variant if there was a problem.
  \return The data for the channel.
  */
  CoreInterfaceData* data() const;  

  //################################################################################################
  nlohmann::json saveState() const;

  //################################################################################################
  void loadState(const nlohmann::json& j, CoreInterface* coreInterface);
};

//##################################################################################################
//! Used to sort a list of handles
/*!
This will sort handles based on there nameID

\param lhs - Left hand side of the less than.
\param rhs - Right hand side of the less than.
\return true if lhs is less than rhs
*/
bool TP_CONTROL_SHARED_EXPORT lessThanCoreInterfaceHandle(const CoreInterfaceHandle& lhs, const CoreInterfaceHandle& rhs);

//##################################################################################################
//! This defines an interface that can be used to communicate between components
/*!
This class allows for communication between modules that have no dependecies on each other, this
includes common modules and application speciffic code.

There are two modes of communication: signals, and channels.

<b>Signals</b><br>
Signals brodcast a message; anything that is listening will receive the message, after that the
message is gone. This is usefull for connecting actions triggered by the user deep in common
modules, up to the end application.

<b>Channels</b><br>
Channels are defined by a type and a name and usually referenced by a handle, they can be set with
a value that is remembered until it is over written. They are useful for holding a persistent state,
there are often multiple channels of the same type each with a differnt name. The user can select
what channel a widget or algorithm should listen to.

There are callbacks that notify of both changes to signals and channels.

The following widgets can be used to interact with core interfaces:
<ul>
<li>\link ToolBar \endlink - Sets the value of a channel based on the selected button.
<li>\link SelectChannelCombo \endlink - Allows the user to select a channel.
</ul>
*/
class TP_CONTROL_SHARED_EXPORT CoreInterface
{
public:
  //################################################################################################
  CoreInterface();

  //################################################################################################
  ~CoreInterface();


  //################################################################################################
  //## Channels ####################################################################################
  //################################################################################################

  //################################################################################################
  //! View all channels
  /*!
  This will return all of the channels that have been defined or set.
  \return All of the channels.
  */
  std::unordered_map<tp_utils::StringID, std::unordered_map<tp_utils::StringID, CoreInterfaceHandle>> channels()const;

  //################################################################################################
  //! Register a channel list changed callback
  /*!
  Callbacks registered with this function will be called when the list of channels changes.

  \param callback - A pointer to the function that should be called.
  \param opaque - This will be passed into the callback.
  */
  void registerCallback(const ChannelListChangedCallback* callback);

  //################################################################################################
  //! Unregister a channel list changed callback
  /*!
  \param callback - A pointer to the callback function.
  \param opaque - This should be the same as the opaque passed into registerCallback().
  */
  void unregisterCallback(const ChannelListChangedCallback* callback);

  //################################################################################################
  //! Get a handle for a channel
  /*!
  The type of a channel usually relates to the data type that channel will contain.

  \param typeID - The type of the channel.
  \param nameID - The name of the channel.
  \return The handle for type and name.
  */
  CoreInterfaceHandle handle(const tp_utils::StringID& typeID, const tp_utils::StringID& nameID);

  //################################################################################################
  //! Register a callback that will be called when a channel changes
  /*!
  Each time a channel is modified all callbacks that have been registered via this method will be
  called. The opaque pointer can be used to identify what instance of your class should be receiving
  the callback.

  You should unregister callbacks when you destroy your class or when you no longer need to receive
  notifications of changes.

  \param callback - A pointer to the function that you want to be called.
  \param opaque - A pointer that will be passed into the callback.

  \sa unregisterCallback()
  */
  void registerCallback(const ChannelChangedCallback* callback);

  //################################################################################################
  //! Unregister a channel changed callback
  /*!
  \param callback - A pointer to the callback.
  \param opaque - Should be the same as the opaque passed into registerCallback().
  */
  void unregisterCallback(const ChannelChangedCallback* callback);

  //################################################################################################
  //! Set the alue held by a channel
  /*!
  This will set the alue of a channel and cause the channel changed callbacks to be called.

  \param handle - The handle of the channel that you want to set.
  \param data - The new value for that channel, this will take ownership.
  */
  void setChannelData(const CoreInterfaceHandle& handle, CoreInterfaceData* data);


  //################################################################################################
  //## Signals #####################################################################################
  //################################################################################################

  //################################################################################################
  //! Register a callback for a signal
  /*!
  If you register a callback you should unregister it when you are done by calling
  unregisterCallback()

  \sa sendSignal()
  \sa unregisterCallback()

  \param callback - The function pointer that will be called when a signal is sent.
  \param typeID - The type of signal that you are interested in.
 */
  void registerCallback(const SignalCallback* callback, const tp_utils::StringID& typeID);

  //################################################################################################
  //! Unregister a signal callback
  /*!
  \param callback - A pointer to the callback function.
  \param typeID - The type that you want to unregister from.
  */
  void unregisterCallback(const SignalCallback* callback, const tp_utils::StringID& typeID);

  //################################################################################################
  //! Send a signal
  /*!
  This will send a signal to all callbacks registered to receive signals of this type. Once the
  signal is despatched to all the callbacks it is forgotten.

  \param typeID The type of signal.
  \param data The payload of the signal or nullptr, this will take ownership.
  */
  void sendSignal(const tp_utils::StringID& typeID, CoreInterfaceData* data);

private:
  struct Private;
  friend struct Private;
  Private* d;
};

}

#endif
