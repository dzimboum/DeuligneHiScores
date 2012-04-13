/*
 * DeuligneHiScores.h - DeuligneHiScores library
 * Copyright 2012 dzimboum.  All rights reserved
 *
 * Released under the WTFPL 2.0 license
 * http://sam.zoy.org/wtfpl/COPYING
 *
 * This library is designed to help storing game scores
 * on EEPROM.  It uses a Deuligne LCD display, shipped
 * by Snootlab.
 */

#ifndef DEULIGNEHISCORES_H
#define DEULIGNEHISCORES_H

#include "Arduino.h"

#include <inttypes.h>

class Deuligne;

/*
 * As EEPROM supports only 100k writes, we want to minimize the
 * number of bytes written onto disk.  For this reason, when a new
 * score is inserted into EEPROM, it overwrites the score which is
 * no more kept, instead of rewriting everything.
 * Scores on disk are thus not sorted, we need a way to sort them
 * when reading from EEPROM.  There is a problem with similar
 * scores, we have to know in which order they have been entered.
 * We will use the 4 least significant bits to store this
 * information; the 28 remaining bits is enough to store game
 * scores.
 * As the number of scores is small, scores also do not have to
 * be sorted in memory.
 */
class DeuligneHiScore
{
public:
  unsigned long value;
  char name[3];
};

class DeuligneHiScores
{
public:
  DeuligneHiScores(Deuligne &lcd);
  void begin(unsigned int number = 5, unsigned int address = 0, unsigned int magic = 0xC15E);
  // Insert score.  If return value is true, score has been inserted,
  // otherwise it was not a high score.
  bool insert(unsigned long score, bool checkOnly = false);
  // Display high scores
  void display();
  // Reset all scores to 0
  void reset();
private:
  void confirmOverwrite();
  void enterName(int index);
  void writeScore(int index);

private:
  unsigned int number_;
  unsigned int address_;
  unsigned int magic_;
  DeuligneHiScore scores[10];
  Deuligne &lcd_;

  // In order to not overwrite important data, scores are written
  // only if the magic number has been found, or if the region
  // was empty.
  bool validMagicNumber_;
  // Set to true when magic number has to be written.
  bool writeMagic_;
};

#endif
