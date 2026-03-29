#ifndef MODELS_USER_H
#define MODELS_USER_H

#include "libvector/libvector.h"
#include "./ranking.h"

// Define database model.
struct UserModel {
  char userId[17];
  char userPassword[17];
  int userCount;
  struct RankingModel* userRanking;
};

#endif