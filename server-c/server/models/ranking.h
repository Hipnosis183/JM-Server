#ifndef MODELS_RANKING_H
#define MODELS_RANKING_H

// Define database model.
struct RankingModel {
  char rankingUser[17];
  char rankingMode[2];
  char rankingScore[10];
  char rankingJewel[5];
  char rankingLevel[4];
  char rankingClass[4];
  char rankingTime[7];
  char rankingDate[20];
};

#endif