//
// Created on Traw-GG 27.08.2025.
//

#include "cHandlingDataMgr.h"
#include "CFileMgr.h"
#include "main.h"
#include "FileLoader.h"
#include "util/patch.h"
#include "Models/ModelInfo.h"
#include <vector>
#include <fstream>

bool isHandlingLoaded = false;

void (*LoadHandlingData_orig)(defHandlingDataMgr *thiz);
void cHandlingDataMgr::LoadHandlingData(defHandlingDataMgr *thiz)
{
    CFileLoader::LoadVehicleObject();

    isHandlingLoaded = true;
    LoadHandlingData_orig(thiz);
    isHandlingLoaded = false;

    const auto pFile = CFileMgr::OpenFile("SAMP/handling.cfg", "rb");
    for (auto line = CFileLoader::LoadLine(pFile); line; line = CFileLoader::LoadLine(pFile))
    {
        std::string_view sv(line);
        if (strncmp(line, ";the end", 8) == 0)
            return;

        if (strlen(line) == 0 || line[0] == ';') {
            // Пропустить комментарии и пустые строки
            continue;
        }

        switch (line[0]) {
            case ';': {
                break; // Comment
            }
                /*case '!': {
                    // bike
                    tBikeHandlingData d{};
                    d.InitFromData(id, line);

                    m_aBikeHandling[id] = d;
                    break;
                }*/
            case '$': {
                std::istringstream iss(line);
                std::string firstSym;
                std::string name;

                iss >> firstSym;
                if (!(iss >> name)) {
                    continue;
                }

                auto id = FindExactWord(name.c_str());
                if (id == -1) {
                    continue;
                }

                Log("id: %d, name: %s", id, name.c_str());
                // flying
                tFlyingHandlingData d{};
                d.InitFromData(id, line);

                m_aFlyingHandling[id] = d;
                continue;
            }
            case '%': {
                std::istringstream iss(line);
                std::string firstSym;
                std::string name;

                iss >> firstSym;
                if (!(iss >> name)) {
                    continue;
                }

                auto id = FindExactWord(name.c_str());
                if (id == -1) {
                    continue;
                }

                // boat
                tBoatHandlingData d{};
                d.InitFromData(id, line);

                m_aBoatHandling[id] = d;
                break;
            }
                /*default: {
                    tHandlingData d{};
                    d.InitFromData(id, line);

                    m_aVehicleHandling[id] = d;
                    break;
                }*/
        }
    }
    CFileMgr::CloseFile(pFile);
}

int32 cHandlingDataMgr::FindExactWord(const char* name) {
    auto it = CVehicleNames::VehicleNames.find(name);
    if (it != CVehicleNames::VehicleNames.end()) {
        return it->second - 400;
    }

    Log("Can't find handling %s", name);
    return -1;
}

int32 cHandlingDataMgr::FindExactWord(const char* name, const char* nameTable, uint32 entrySize, uint32 entryCount) {
    for (auto i = 0u; i < entryCount; i++) {
        const auto entry = &nameTable[entrySize * i];
        if (!strncmp(name, entry, strlen(entry))) {
            return i;
        }
    }
    Log("Vehicle name not found in table: %s", name);
    return -1;
}

int32 tHandlingData::InitFromData(int32 id, const char* line) {
    m_nVehicleId = id;
    Log("InitFromData veh %d, line = %s", id, line);
    const auto n = sscanf(
            line,
            "%*s %f %f %f%f\t%f\t%f\t%hhu\t%f\t%f\t%f\t%hhu\t%f\t%f\t%f\t%c\t%c\t%f\t%f\t%hhu\t%f\t%f\t%f\t%f\t%f\t%f\t%f\t%f\t%f\t%f\t%d\t%x\t%x\t%hhu\t%hhu\t%hhu",
            &m_fMass, // 1
            &m_fTurnMass,
            &m_fDragMult,

            &m_vecCentreOfMass.x, // 4
            &m_vecCentreOfMass.y,
            &m_vecCentreOfMass.z,

            &m_nPercentSubmerged, // 7

            &m_fTractionMultiplier, // 8
            &m_fTractionLoss,
            &m_fTractionBias,

            &m_transmissionData.m_nNumberOfGears, // 11
            &m_transmissionData.m_fMaxGearVelocity,
            &m_transmissionData.m_fEngineAcceleration,
            &m_transmissionData.m_fEngineInertia,
            &m_transmissionData.m_nDriveType,
            &m_transmissionData.m_nEngineType,

            &m_fBrakeDeceleration, // 18
            &m_fBrakeBias,
            &m_bABS,
            &m_fSteeringLock,
            &m_fSuspensionForceLevel,

            &m_fSuspensionDampingLevel, // 22
            &m_fSuspensionHighSpdComDamp,
            &m_fSuspensionUpperLimit,
            &m_fSuspensionLowerLimit,
            &m_fSuspensionBiasBetweenFrontAndRear,
            &m_fSuspensionAntiDiveMultiplier,

            &m_fSeatOffsetDistance,
            &m_fCollisionDamageMultiplier,

            &m_nMonetaryValue, // 31
            &m_nModelFlags,
            &m_nHandlingFlags,
            &m_nFrontLights,
            &m_nRearLights,
            &m_nAnimGroup
    );
    m_transmissionData.m_handlingFlags = m_nHandlingFlags;
    m_transmissionData.m_fEngineAcceleration *= 0.4f;
    cHandlingDataMgr::ConvertDataToGameUnits(this);
    return n == 35 ? -1 : n;
}

int32 tBoatHandlingData::InitFromData(int32 id, const char* line) {
    m_nVehicleId = id;

    std::istringstream iss(line);
    std::string dummy;
    iss >> dummy; // Пропускаем первое слово

    std::string dummy2;
    iss >> dummy2; // Пропускаем второе слово

    iss >> m_fThrustY
        >> m_fThrustZ
        >> m_fThrustAppZ
        >> m_fAqPlaneForce
        >> m_fAqPlaneLimit
        >> m_fAqPlaneOffset
        >> m_fWaveAudioMult
        >> m_vecMoveRes.x
        >> m_vecMoveRes.y
        >> m_vecMoveRes.z
        >> m_vecTurnRes.x
        >> m_vecTurnRes.y
        >> m_vecTurnRes.z
        >> m_fLookLRBehindCamHeight;

    return iss.eof() ? -1 : 14 - static_cast<int32>(iss.tellg() / sizeof(float));
}

int32 tFlyingHandlingData::InitFromData(int32 id, const char* line) {
    m_nVehicleId = id;

    std::istringstream iss(line);
    std::string dummy;
    iss >> dummy; // Пропускаем первое слово

    std::string dummy2;
    iss >> dummy2; // Пропускаем второе слово

    // Для обработки случая с лишним символом 's' в RCRAIDER
    if (dummy == "RCRAIDER") {
        char extraChar;
        iss >> extraChar; // Пропускаем лишний символ
    }

    iss >> m_fThrust
        >> m_fThrustFallOff
        >> m_fYaw
        >> m_fYawStab
        >> m_fSideSlip
        >> m_fRoll
        >> m_fRollStab
        >> m_fPitch
        >> m_fPitchStab
        >> m_fFormLift
        >> m_fAttackLift
        >> m_fGearUpR
        >> m_fGearDownL
        >> m_fWindMult
        >> m_fMoveRes
        >> m_vecTurnRes.x
        >> m_vecTurnRes.y
        >> m_vecTurnRes.z
        >> m_vecSpeedRes.x
        >> m_vecSpeedRes.y
        >> m_vecSpeedRes.z;

    return iss.eof() ? -1 : 21 - static_cast<int32>(iss.tellg() / sizeof(float));
}

int32 tBikeHandlingData::InitFromData(int32 id, const char* line) {
    m_nVehicleId = id;

    const auto n = sscanf(
            line,
            "%*s\t%f\t%f\t%f\t%f\t%f\t%f\t%f\t%f\t%f\t%f\t%f\t%f\t%f\t%f\t%f",
            &m_fLeanFwdCOM,
            &m_fLeanFwdForce,
            &m_fLeanBakCOM,
            &m_fLeanBakForce,
            &m_fMaxLean,
            &m_fFullAnimLean,
            &m_fDesLean,
            &m_fSpeedSteer,
            &m_fSlipSteer,
            &m_fNoPlayerCOMz,
            &m_fWheelieAng,
            &m_fStoppieAng,
            &m_fWheelieSteer,
            &m_fWheelieStabMult,
            &m_fStoppieStabMult
    );
    // gHandlingDataMgr.ConvertBikeDataToGameUnits(this);
    return n == 15 ? -1 : n;
}

void cHandlingDataMgr::ConvertDataToGameUnits(tHandlingData* h) {
    CHook::CallFunction<void>(g_libGTASA + (VER_x32 ? 0x00570DC8 + 1 : 0x69343C), 0, h);
}

void cHandlingDataMgr::ConvertBikeDataToGameUnits(tBikeHandlingData* h) {
    CHook::CallFunction<void>(g_libGTASA + (VER_x32 ? 0x00571008 + 1 : 0x69368C), 0, h);
}

// get handling id by name
int32 cHandlingDataMgr::GetHandlingId(const char* nameToFind) {
    auto it = CVehicleNames::VehicleNames.find(nameToFind);
    if (it != CVehicleNames::VehicleNames.end()) {
        return it->second - 400;
    }

    DLOG("Can't find handling %s", nameToFind);
    return 0;
}

tFlyingHandlingData* cHandlingDataMgr::GetFlyingPointer(uint8 handlingId) {
    auto it = m_aFlyingHandling.find(handlingId);
    return it != m_aFlyingHandling.end() ? &it->second : nullptr;
}

// 0x6F5300
tBoatHandlingData* cHandlingDataMgr::GetBoatPointer(uint8 handlingId) {
    auto it = m_aBoatHandling.find(handlingId);
    return it != m_aBoatHandling.end() ? &it->second : nullptr;
}

tHandlingData* cHandlingDataMgr::GetVehiclePointer(uint32 handlingId) {
    auto it = m_aVehicleHandling.find(handlingId);
    return it != m_aVehicleHandling.end() ? &it->second : nullptr;
}

tBikeHandlingData* cHandlingDataMgr::GetBikeHandlingPointer(uint32 handlingId) {
    auto it = m_aBikeHandling.find(handlingId);
    return it != m_aBikeHandling.end() ? &it->second : nullptr;
}

int32 GetHandlingId_hooked(uintptr* thiz, const char* nameToFind) {
    return cHandlingDataMgr::GetHandlingId(nameToFind);
}

tFlyingHandlingData* (*defHandlingDataMgr__GetFlyingPointer_orig)(defHandlingDataMgr* thiz, uint8 plane);
tFlyingHandlingData* defHandlingDataMgr__GetFlyingPointer(defHandlingDataMgr* thiz, uint8 plane) {
    if (isHandlingLoaded) {
        return defHandlingDataMgr__GetFlyingPointer_orig(thiz, plane);
    }
    return cHandlingDataMgr::GetFlyingPointer(plane);
}

tBoatHandlingData* (*defHandlingDataMgr__GetBoatPointer_orig)(defHandlingDataMgr* thiz, uint8 boat);
tBoatHandlingData* defHandlingDataMgr__GetBoatPointer(defHandlingDataMgr* thiz, uint8 boat) {
    if (isHandlingLoaded) {
        return defHandlingDataMgr__GetBoatPointer_orig(thiz, boat);
    }
    return cHandlingDataMgr::GetBoatPointer(boat);
}

void cHandlingDataMgr::InjectHooks() {
    CHook::Redirect("_ZN16cHandlingDataMgr13GetHandlingIdEPKc", &GetHandlingId_hooked);
    CHook::InlineHook("_ZN16cHandlingDataMgr16LoadHandlingDataEv", &LoadHandlingData, &LoadHandlingData_orig);
    CHook::InlineHook("_ZN16cHandlingDataMgr14GetBoatPointerEh", &defHandlingDataMgr__GetBoatPointer, &defHandlingDataMgr__GetBoatPointer_orig);
    CHook::InlineHook("_ZN16cHandlingDataMgr16GetFlyingPointerEh", &defHandlingDataMgr__GetFlyingPointer, &defHandlingDataMgr__GetFlyingPointer_orig);
}
