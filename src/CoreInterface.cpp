#include "tp_control/CoreInterface.h"

namespace tp_control
{
//##################################################################################################
struct CoreInterfacePayloadPrivate
{
  CoreInterfaceData* data{nullptr};

  ~CoreInterfacePayloadPrivate()
  {
    delete data;
  }
};

//##################################################################################################
CoreInterfaceHandle::CoreInterfaceHandle(tp_utils::StringID typeID, tp_utils::StringID nameID):
  m_typeID(std::move(typeID)),
  m_nameID(std::move(nameID))
{
}

//##################################################################################################
CoreInterfaceHandle::CoreInterfaceHandle()
{

}

//##################################################################################################
const tp_utils::StringID& CoreInterfaceHandle::typeID() const
{
  return m_typeID;
}

//##################################################################################################
const tp_utils::StringID& CoreInterfaceHandle::nameID() const
{
  return m_nameID;
}

//##################################################################################################
bool CoreInterfaceHandle::operator==(const CoreInterfaceHandle& other)const
{
  return m_payload == other.m_payload;
}

//##################################################################################################
CoreInterfaceData* CoreInterfaceHandle::data() const
{
  return m_payload?m_payload->data:nullptr;
}

//##################################################################################################
bool lessThanCoreInterfaceHandle(const CoreInterfaceHandle& lhs, const CoreInterfaceHandle& rhs)
{
  return lhs.nameID().keyString() < rhs.nameID().keyString();
}

//##################################################################################################
struct CoreInterface::Private
{
  std::unordered_map<tp_utils::StringID, std::unordered_map<tp_utils::StringID, CoreInterfaceHandle>> channels;

  std::vector<const ChannelChangedCallback*> channelChangeCallbacks;
  std::vector<const ChannelListChangedCallback*> channelListChangedCallbacks;
  std::unordered_map<tp_utils::StringID, std::vector<const SignalCallback*>> signalCallbacks;

  ~Private()
  {
    for(const auto& i : channels)
      for(const auto& j : i.second)
        delete j.second.m_payload;
  }
};

//##################################################################################################
CoreInterface::CoreInterface():
  d(new Private())
{

}

//##################################################################################################
CoreInterface::~CoreInterface()
{
  delete d;
}

//##################################################################################################
std::unordered_map<tp_utils::StringID, std::unordered_map<tp_utils::StringID, CoreInterfaceHandle>> CoreInterface::channels()const
{
  return d->channels;
}

//##################################################################################################
void CoreInterface::registerCallback(const ChannelListChangedCallback* callback)
{
  d->channelListChangedCallbacks.push_back(callback);
}

//##################################################################################################
void CoreInterface::unregisterCallback(const ChannelListChangedCallback* callback)
{
  tpRemoveOne(d->channelListChangedCallbacks, callback);
}

//##################################################################################################
CoreInterfaceHandle CoreInterface::handle(const tp_utils::StringID& typeID, const tp_utils::StringID& nameID)
{
  if(!typeID.isValid() || !nameID.isValid())
    return CoreInterfaceHandle();

  CoreInterfaceHandle& localHandle = d->channels[typeID][nameID];

  if(!localHandle.m_payload)
  {
    localHandle.m_payload = new CoreInterfacePayloadPrivate;
    localHandle.m_typeID = typeID;
    localHandle.m_nameID = nameID;

    for(const auto& c : d->channelListChangedCallbacks)
      (*c)();
  }

  return localHandle;
}

//##################################################################################################
void CoreInterface::registerCallback(const ChannelChangedCallback* callback)
{
  d->channelChangeCallbacks.push_back(callback);
}

//##################################################################################################
void CoreInterface::unregisterCallback(const ChannelChangedCallback* callback)
{
  tpRemoveOne(d->channelChangeCallbacks, callback);
}

//##################################################################################################
void CoreInterface::setChannelData(const CoreInterfaceHandle& handle, CoreInterfaceData* data)
{
  if(!handle.m_payload)
  {
    delete data;
    return;
  }

  if(handle.m_payload->data)
    delete handle.m_payload->data;

  handle.m_payload->data = data;

  for(const auto& c : d->channelChangeCallbacks)
    (*c)(handle.m_typeID, handle.m_nameID, handle.m_payload->data);
}

//##################################################################################################
void CoreInterface::registerCallback(const SignalCallback* callback, const tp_utils::StringID& typeID)
{
  d->signalCallbacks[typeID].push_back(callback);
}

//##################################################################################################
void CoreInterface::unregisterCallback(const SignalCallback* callback, const tp_utils::StringID& typeID)
{
  auto& callbacks = d->signalCallbacks[typeID];
  tpRemoveOne(callbacks, callback);
  if(callbacks.empty())
    d->signalCallbacks.erase(typeID);
}

//##################################################################################################
void CoreInterface::sendSignal(const tp_utils::StringID& typeID, CoreInterfaceData* data)
{
  for(const auto& c : d->signalCallbacks[typeID])
    (*c)(typeID, data);
}

}