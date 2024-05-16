#include "tp_control/CoreInterface.h"

#include "tp_utils/JSONUtils.h"

#include <thread>
#include <cassert>

namespace tp_control
{
//##################################################################################################
struct CoreInterfacePayloadPrivate
{
  TP_NONCOPYABLE(CoreInterfacePayloadPrivate);

  CoreInterfaceData* data{nullptr};

  CoreInterfacePayloadPrivate()=default;

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
void CoreInterfaceHandle::saveState(nlohmann::json& j) const
{
  j["typeID"] = m_typeID.toString();
  j["nameID"] = m_nameID.toString();
}

//##################################################################################################
void CoreInterfaceHandle::loadState(const nlohmann::json& j, CoreInterface* coreInterface)
{
  (*this) = coreInterface->handle(TPJSONString(j, "typeID"), TPJSONString(j, "nameID"));
}

//##################################################################################################
bool lessThanCoreInterfaceHandle(const CoreInterfaceHandle& lhs, const CoreInterfaceHandle& rhs)
{
  return lhs.nameID().toString() < rhs.nameID().toString();
}

//##################################################################################################
struct CoreInterface::Private
{
  TP_NONCOPYABLE(Private);

  std::thread::id ownerThread{std::this_thread::get_id()};

  std::unordered_map<tp_utils::StringID, std::unordered_map<tp_utils::StringID, CoreInterfaceHandle>> channels;

  std::unordered_map<tp_utils::StringID, std::vector<const SignalCallback*>> signalCallbacks;

  Private()=default;

  //################################################################################################
  ~Private()
  {
    for(const auto& i : channels)
      for(const auto& j : i.second)
        delete j.second.m_payload;
  }

  //################################################################################################
  void checkThread()
  {
    assert(ownerThread==std::this_thread::get_id());
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
  d->checkThread();
  delete d;
}

//##################################################################################################
const std::unordered_map<tp_utils::StringID, std::unordered_map<tp_utils::StringID, CoreInterfaceHandle>>& CoreInterface::channels()const
{
  d->checkThread();
  return d->channels;
}

//##################################################################################################
CoreInterfaceHandle CoreInterface::handle(const tp_utils::StringID& typeID, const tp_utils::StringID& nameID)
{
  d->checkThread();
  if(!typeID.isValid() || !nameID.isValid())
    return CoreInterfaceHandle();

  CoreInterfaceHandle& localHandle = d->channels[typeID][nameID];
  if(!localHandle.m_payload)
  {
    localHandle.m_payload = new CoreInterfacePayloadPrivate;
    localHandle.m_typeID = typeID;
    localHandle.m_nameID = nameID;

    channelListChanged();
  }

  return localHandle;
}

//##################################################################################################
void CoreInterface::setChannelData(const CoreInterfaceHandle& handle, CoreInterfaceData* data)
{
  d->checkThread();
  if(!handle.m_payload)
  {
    delete data;
    return;
  }

  delete handle.m_payload->data;

  handle.m_payload->data = data;

  channelChanged(handle.m_typeID, handle.m_nameID, handle.m_payload->data);
}

//##################################################################################################
void CoreInterface::registerCallback(const SignalCallback* callback, const tp_utils::StringID& typeID)
{
  d->checkThread();
  d->signalCallbacks[typeID].push_back(callback);
}

//##################################################################################################
void CoreInterface::unregisterCallback(const SignalCallback* callback, const tp_utils::StringID& typeID)
{
  d->checkThread();
  auto& callbacks = d->signalCallbacks[typeID];
  tpRemoveOne(callbacks, callback);
  if(callbacks.empty())
    d->signalCallbacks.erase(typeID);
}

//##################################################################################################
void CoreInterface::sendSignal(const tp_utils::StringID& typeID, CoreInterfaceData* data)
{
  d->checkThread();
  for(const auto& c : d->signalCallbacks[typeID])
    (*c)(typeID, data);
}

}
