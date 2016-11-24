
#include <nvapi.h>
#include <NvApiDriverSettings.h>
#include <string>
#include <vector>
#include <iostream>

#pragma comment(lib, "nvapi.lib" )

namespace nv{ namespace info {

void print_error(NvAPI_Status status)
{
  NvAPI_ShortString error;
  NvAPI_GetErrorMessage(status, error);
  std::cout << error << "\n";
}

void print_setting_info(NVDRS_SETTING & setting)
{
  ::NvAPI_DRS_GetSettingNameFromId(setting.settingId, &setting.settingName);
  wprintf(L"Setting Name: %s\n", setting.settingName);
  printf("Setting ID: %X\n", setting.settingId);
  printf("Predefined? : %d\n", setting.isCurrentPredefined);
  switch (setting.settingType) {
  case NVDRS_DWORD_TYPE:
    printf("Setting Value: %X\n", setting.u32CurrentValue);
    break;
  case NVDRS_BINARY_TYPE: {
    unsigned int len;
    printf("Setting Binary (length=%d) :", setting.binaryCurrentValue.valueLength);
    for (len = 0; len < setting.binaryCurrentValue.valueLength; len++) {
      printf(" %02x", setting.binaryCurrentValue.valueData[len]);
    }
    printf("\n");
  } break;
  case NVDRS_WSTRING_TYPE:
    wprintf(L"Setting Value: %s\n", setting.wszCurrentValue);
    break;
  }
}

bool display_profile_contents( NvDRSSessionHandle hSession, NvDRSProfileHandle hProfile )
{
  NvAPI_Status status;

  NVDRS_PROFILE profileInformation = { NVDRS_PROFILE_VER };

  status = ::NvAPI_DRS_GetProfileInfo(hSession, hProfile, &profileInformation);
  if (status != NVAPI_OK) {
    return false;
  }
  wprintf(L"Profile Name: %s\n", profileInformation.profileName);
  printf("Number of Applications associated with the Profile: %d\n", profileInformation.numOfApps);
  printf("Number of Settings associated with the Profile: %d\n", profileInformation.numOfSettings);
  printf("Is Predefined: %d\n", profileInformation.isPredefined);

  if (profileInformation.numOfApps > 0) {
    std::vector<NVDRS_APPLICATION> appArray(profileInformation.numOfApps);

    NvU32 numAppsRead = profileInformation.numOfApps;
    NvU32 i;

    appArray[0] = { NVDRS_APPLICATION_VER };

    status = ::NvAPI_DRS_EnumApplications(hSession, hProfile, 0, &numAppsRead, appArray.data());
    if (status != NVAPI_OK) {
      print_error(status);
      return false;
    }
    for (i = 0; i < numAppsRead; i++) {
      wprintf(L"Executable: %s\n", appArray[i].appName);
      wprintf(L"User Friendly Name: %s\n", appArray[i].userFriendlyName);
      printf("Is Predefined: %d\n", appArray[i].isPredefined);
    }
  }
  if (profileInformation.numOfSettings > 0) {
    std::vector<NVDRS_SETTING> setArray(profileInformation.numOfSettings);
    NvU32          numSetRead = profileInformation.numOfSettings, i;
    setArray[0] = { NVDRS_SETTING_VER };

    status = ::NvAPI_DRS_EnumSettings(hSession, hProfile, 0, &numSetRead, setArray.data());
    if (status != NVAPI_OK) {
      print_error(status);
      return false;
    }
    for (i = 0; i < numSetRead; i++) {
      NVDRS_SETTING & setting = setArray[i];

      if (setting.settingLocation != NVDRS_CURRENT_PROFILE_LOCATION) {
        continue;
      }
      print_setting_info(setting);
    }
  }
  printf("\n");
  return true;
}

void enumerate_profiles_on_system()
{
  NvAPI_Status status;
  status = NvAPI_Initialize();
  if (status != NVAPI_OK)
    print_error(status);

  NvDRSSessionHandle hSession = 0;

  status = ::NvAPI_DRS_CreateSession(&hSession);
  if (status != NVAPI_OK)
    print_error(status);

  status = ::NvAPI_DRS_LoadSettings(hSession);
  if (status != NVAPI_OK)
    print_error(status);

  NvDRSProfileHandle hProfile = 0;
  unsigned int       index    = 0;
  while ((status = ::NvAPI_DRS_EnumProfiles(hSession, index, &hProfile)) == NVAPI_OK) {
    printf("Profile in position %d:\n", index);
    display_profile_contents(hSession, hProfile);
    index++;
  }
  if (status == NVAPI_END_ENUMERATION) {
    // this is expected at the end of the enumeration
  } else if (status != NVAPI_OK)
    print_error(status);

  ::NvAPI_DRS_DestroySession(hSession);
  hSession = 0;
}

}} //namespace nv::info

int main()
{
  NvDRSSessionHandle _session{};
  NvDRSProfileHandle _profile{};
  NVDRS_PROFILE      _profInfo{};

  if (NvAPI_Initialize() != NVAPI_OK)
    throw std::runtime_error("NVIDIA Api not initialized");

  if (::NvAPI_DRS_CreateSession(&_session) != NVAPI_OK)
    throw std::runtime_error("can't create session");

  if (::NvAPI_DRS_LoadSettings(_session) != NVAPI_OK)
    throw std::runtime_error("can't load system settings");

  if (::NvAPI_DRS_GetCurrentGlobalProfile(_session, &_profile) != NVAPI_OK)
    throw std::runtime_error("can't get current global profile");

  NvPhysicalGpuHandle nvGPUHandle[NVAPI_MAX_PHYSICAL_GPUS];
  NvAPI_Status        status;
  NvU32               GpuCount;
  NvU32               DeviceId;
  NvU32               SubSystemId;
  NvU32               RevisionId;
  NvU32               ExtDeviceId;
  NvU32               BusId;
  NvU32               BiosRevision;
  NvU32               BiosRevisionOEM;
  NV_BOARD_INFO       BoardInfo{ NV_BOARD_INFO_VER };
  NvU32               ConfiguredFeatureMask;
  NvU32               ConsistentFeatureMask;
  NvU32               CoreCount;

  NvAPI_EnumPhysicalGPUs(nvGPUHandle, &GpuCount);

  NvAPI_ShortString str{};
  NvAPI_GPU_GetFullName          (nvGPUHandle[0], str);
  NvAPI_GPU_GetPCIIdentifiers    (nvGPUHandle[0], &DeviceId, &SubSystemId, &RevisionId, &ExtDeviceId);
  NvAPI_GPU_GetBusId             (nvGPUHandle[0], &BusId);
  NvAPI_GPU_GetVbiosRevision     (nvGPUHandle[0], &BiosRevision);
  NvAPI_GPU_GetVbiosOEMRevision  (nvGPUHandle[0], &BiosRevisionOEM);
  NvAPI_GPU_GetVbiosVersionString(nvGPUHandle[0], str);

  status = NvAPI_GPU_GetBoardInfo            (nvGPUHandle[0], &BoardInfo);
  status = NvAPI_GPU_WorkstationFeatureQuery (nvGPUHandle[0], &ConfiguredFeatureMask, &ConsistentFeatureMask);
  status = NvAPI_GPU_GetGpuCoreCount         (nvGPUHandle[0], &CoreCount);

  NV_CHIPSET_INFO info{ NV_CHIPSET_INFO_VER_4 };

  status = NvAPI_SYS_GetChipSetInfo(&info);
  NvAPI_GetInterfaceVersionString(str);

  nv::info::display_profile_contents(_session, _profile);

  //if (::NvAPI_DRS_GetBaseProfile(_session, &_profile) != NVAPI_OK)
  //  throw std::runtime_error("can't get current global profile");

  //::NvAPI_DRS_RestoreProfileDefault(_session, _profile);
  //DisplayProfileContents(_session, _profile);

  //::NvAPI_DRS_SaveSettings(_session);

  // unsigned int test  = (unsigned int)-0x68;
  // unsigned int index = 0;
  // while ((status = ::NvAPI_DRS_EnumProfiles(_session, index, &_profile)) == NVAPI_OK) {
  //  _profInfo.version = NVDRS_PROFILE_VER;

  //  DisplayProfileContents(_session, _profile);

  //  //if (::NvAPI_DRS_GetProfileInfo(_session, _profile, &_profInfo) != NVAPI_OK)
  //  //  throw std::runtime_error("can't get current global profile info");

  //  index++;
  //}

  //nv::info::enumerate_profiles_on_system();

  ::NvAPI_DRS_DestroySession(_session);

  return 0;
}