#pragma once
#include "game.h"
namespace armlet {bool Tick(const Frame&);bool Pending();bool OwnsLane(const Frame&);void Reset();const char* Status();
const char* StatusRu();
const char* LastClosedStatus();
const char* SelectionDiagnostics();
const char* DangerDiagnostics();
const char* CycleDiagnostics();
const char* DamageDiagnostics();
const char* TraceDiagnostics();}
