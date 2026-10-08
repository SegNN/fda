#pragma once
#include "game.h"
#include "kill_helper_core.h"
#include <vector>
namespace killhelper {
struct Row {char label[96]={};int level=0;bool modeled=false,manual=false;killhelpercore::Result result;};
struct Report {bool target=false;char name[32]={};int hp=0;std::vector<Row> rows;};
Report Build(const Frame& f);
void Draw(const Frame& f);
}
