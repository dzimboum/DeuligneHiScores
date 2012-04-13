/*
 * DeuligneHiScores.cpp - DeuligneHiScores library
 * Copyright 2012 dzimboum.  All rights reserved
 *
 * Released under the WTFPL 2.0 license
 * http://sam.zoy.org/wtfpl/COPYING
 *
 * This library is designed to help storing game scores
 * on EEPROM.  It uses a Deuligne LCD display, shipped
 * by Snootlab.
 */

#include <Deuligne.h>
#include <EEPROM.h>

#include <inttypes.h>

#include "DeuligneHiScores.h"
 
#define DEULIGNEHISCORES_DEBUG
#undef DEULIGNEHISCORES_DEBUG

DeuligneHiScores::DeuligneHiScores(Deuligne &lcd)
  : lcd_(lcd)
{
}

/*
 * number: the number of entries (must be between 1 and 10 inclusive)
 * address: the address of EEPROM where scores are stored (4+7*number bytes are written)
 * magic: 2 bytes written at the beginning and end of data, to ensure that we do not
 *        overwrite existing data.
 */
void
DeuligneHiScores::begin(unsigned int number, unsigned int address, unsigned int magic)
{
  number_  = number;
  address_ = address;
  magic_   = magic;
  writeMagic_ = false;
  if (number_ > 5) number_ = 5;
  if (number_ < 1) {
    validMagicNumber_ = false; 
    return;
  }
  // Initialize scores
  for (unsigned int i = 0; i < number_; ++i)
  {
    // The least significant 4 bytes are used to
    // distinguish between equal scores.
    this->scores[i].value = 0x0f - i;
    this->scores[i].name[0] = 'A';
    this->scores[i].name[1] = 'A';
    this->scores[i].name[2] = 'A';
  }

  byte byteL = lowByte(magic_);
  byte byteH = highByte(magic_);
  uint8_t b0 = EEPROM.read(address);
  ++address;
  uint8_t b1 = EEPROM.read(address);
  ++address;
#ifdef DEULIGNEHISCORES_DEBUG
  Serial.print("Reading bytes at address ");
  Serial.println(address_);
  Serial.print((int) b0);
  Serial.print(' ');
  Serial.println((int) b1);
#endif
  if (b0 == byteH && b1 == byteL)
  {
#ifdef DEULIGNEHISCORES_DEBUG
    Serial.println("Reading scores");
#endif
    for (unsigned int i = 0; i < number_; ++i)
    {
      unsigned long value = 0;
      // Score is stored in low-endian order
      for (int j = 0; j < 4; ++j)
      {
        value <<= 8;
        value += EEPROM.read(address + 3 - j);
      }
      address += 4;
      this->scores[i].value = value;
      for (int j = 0; j < 3; ++j)
      {
        this->scores[i].name[j] = EEPROM.read(address);
        ++address;
      }
#ifdef DEULIGNEHISCORES_DEBUG
      Serial.print(this->scores[i].value >> 4);
      Serial.print(" (");
      Serial.print((int) (this->scores[i].value & 0xf));
      Serial.print(") ");
      Serial.print(this->scores[i].name[0]);
      Serial.print(this->scores[i].name[1]);
      Serial.println(this->scores[i].name[2]);
#endif
    }
    b0 = EEPROM.read(address);
    ++address;
    b1 = EEPROM.read(address);
    ++address;
    validMagicNumber_ = (b0 == byteH && b1 == byteL);
#ifdef DEULIGNEHISCORES_DEBUG
    Serial.print("Valid trailing magic bytes? ");
    Serial.println(validMagicNumber_);
#endif
  }
  else if (b0 == 0xff && b1 == 0xff)
  {
    writeMagic_ = true;
#ifdef DEULIGNEHISCORES_DEBUG
    Serial.print("Empty region? ");
#endif
    validMagicNumber_ = true; 
    for (unsigned int i = 0; i < 7*number_; ++i)
    {
      if (EEPROM.read(address + i) != 0xff) {
        validMagicNumber_ = false;
#ifdef DEULIGNEHISCORES_DEBUG
        Serial.println(validMagicNumber_);
        Serial.print("Data found at address ");
        Serial.println(address + i);
#endif
        break;
      }
    }
#ifdef DEULIGNEHISCORES_DEBUG
    if (validMagicNumber_) Serial.println(validMagicNumber_);
#endif
  }
  else
  {
    validMagicNumber_ = false; 
    writeMagic_ = true;
#ifdef DEULIGNEHISCORES_DEBUG
    Serial.print("Magic bytes mismatch, expected bytes are: ");
    Serial.print((int) byteH);
    Serial.print(' ');
    Serial.println((int) byteL);
#endif
  }
  if (!validMagicNumber_) {
    confirmOverwrite();
  }
}

void
DeuligneHiScores::confirmOverwrite()
{
  int key = -1;
  int old_key = -1;
  bool overwrite = false;

  lcd_.clear();
  lcd_.setCursor(0,0);
  lcd_.print("Overwrite data");
  lcd_.setCursor(0,1);
  lcd_.print("on EEPROM? No ");
  while (true) {
    key = lcd_.get_key();
    if (key != old_key) {
      delay(50);
      key = lcd_.get_key();
      if (key != old_key)
      {
        old_key = key;
        if (key >= 0 && key <= 3) {
          overwrite = !overwrite;
          lcd_.setCursor(11,1);
          if (overwrite) {
            lcd_.print("Yes");
          } else {
            lcd_.print("No ");
          }
        } else if (key == 4) {
          validMagicNumber_ = overwrite;
#ifdef DEULIGNEHISCORES_DEBUG
          Serial.print("overwrite EEPROM? ");
          Serial.println(overwrite);
#endif
          return;
        }
      }
    }
  }
}

/*
 * value: score
 * checkOnly: if true, we are only interested by return value,
 *            score is not inserted.
 * Return value: true if this is a high score, false otherwise.
 */
bool
DeuligneHiScores::insert(unsigned long value, bool checkOnly)
{
  value <<= 4;
  bool isBestScore = false;
  for (unsigned int i = 0; i < number_; ++i) {
    if (this->scores[i].value < value) {
      isBestScore = true;
      break;
    }
  }
#ifdef DEULIGNEHISCORES_DEBUG
  Serial.print("Value is ");
  Serial.println(value);
  Serial.print("isBestScore? ");
  Serial.println(isBestScore);
#endif
  if (checkOnly) return isBestScore;
  if (!isBestScore) return false;

  // In order to insert this value, we need the index of the lowest
  // value, which will be overwritten.  We also have to modify
  // value if it is already in the array.
  unsigned long lowest = value;
  int indexLowest = 0;
  int same = 0;
  for (unsigned int i = 0; i < number_; ++i) {
    if ((this->scores[i].value & ~0x0fL) == value) {
      ++same;
    }
    if (this->scores[i].value < lowest) {
      lowest = this->scores[i].value;
      indexLowest = i;
    }
  }
  this->scores[indexLowest].value = value + 0xf - same;
  if (validMagicNumber_) {
    enterName(indexLowest);
    writeScore(indexLowest);
  }
  return true;
}

void
DeuligneHiScores::enterName(int index)
{
  // variable is static to remember previous entered name
  static char name[3] = { 'A', 'A', 'A' };
  
  lcd_.clear();
  lcd_.setCursor(1,0);
  lcd_.println("Enter your name");
  lcd_.setCursor(5,1);
  lcd_.print(name[0]);
  lcd_.print(name[1]);
  lcd_.print(name[2]);
  lcd_.setCursor(5,1);
  lcd_.blink();
  int i = 0;
  int key = -1;
  int old_key = -1;
  while (i < 3) {
    key = lcd_.get_key();
    if (key != old_key) {
      delay(50);
      key = lcd_.get_key();
      if (key != old_key)
      {
        old_key = key;
        if (key == 0 || key == 4) {
          ++i;
          lcd_.setCursor(5+i, 1);
        } else if (key == 3) {
          --i;
          if (i < 0) i = 0;
          lcd_.setCursor(5+i, 1);
        } else if (key == 1) {
          ++name[i];
          if (name[i] > 125) name[i] = 32;
          lcd_.setCursor(5+i, 1);
          lcd_.print(name[i]);
          lcd_.setCursor(5+i, 1);
        } else if (key == 2) {
          --name[i];
          if (name[i] < 32) name[i] = 125;
          lcd_.setCursor(5+i, 1);
          lcd_.print(name[i]);
          lcd_.setCursor(5+i, 1);
        }
      }
    }
  }
  lcd_.noBlink();
  this->scores[index].name[0] = name[0];
  this->scores[index].name[1] = name[1];
  this->scores[index].name[2] = name[2];
}

void
DeuligneHiScores::writeScore(int index)
{
  if (writeMagic_) {
    // Avoid infinite recursion because writeScore is called below
    writeMagic_ = false;
    // Initialize region
    byte byteL = lowByte(magic_);
    byte byteH = highByte(magic_);
    EEPROM.write(address_, byteH);
    EEPROM.write(address_+1, byteL);
    for (int i = 0; i < number_; ++i) {
      writeScore(i);
    }
    EEPROM.write(address_+2+7*number_, byteH);
    EEPROM.write(address_+3+7*number_, byteL);
  }
  unsigned int address = address_ + 2 + index * 7;
  unsigned long value = this->scores[index].value;
#ifdef DEULIGNEHISCORES_DEBUG
  Serial.print("Write score at address ");
  Serial.println(address);
  Serial.print("Bytes for score: ");
#endif
  for (int j = 0; j < 4; ++j)
  {
#ifdef DEULIGNEHISCORES_DEBUG
    Serial.print((int)(value & 0xff));
#endif
    EEPROM.write(address+j, value & 0xff);
    value >>= 8;
  }
  address += 4;
#ifdef DEULIGNEHISCORES_DEBUG
  Serial.println();
  Serial.print("Name: ");
#endif
  for (int j = 0; j < 3; ++j)
  {
#ifdef DEULIGNEHISCORES_DEBUG
    Serial.print(this->scores[index].name[j]);
#endif
    EEPROM.write(address+j, this->scores[index].name[j]);
  }
#ifdef DEULIGNEHISCORES_DEBUG
  Serial.println();
#endif
}

/*
 * Display scores.
 * As they are not sorted in memory, some black magic is now needed.
 * We use the most inefficient sorting algorithm, but since we have
 * to sort less than 10 entries, it does not matter.
 */
void
DeuligneHiScores::display()
{
  unsigned int displayed = 0;
  lcd_.clear();
  lcd_.setCursor(2,0);
  lcd_.print("Hall Of Fame");
  for (int i = 0; i < number_; ++i)
  {
    unsigned int mask = 1;
    unsigned long maxScore = 0;
    int maxIndex = 0;
    for (int j = 0; j < number_; ++j) {
      if ((displayed & mask) == 0) {
        if (this->scores[j].value > maxScore) {
          maxScore = this->scores[j].value;
          maxIndex = j;
        }
      }
      mask <<= 1;
    }
    displayed |= (1 << maxIndex);
    lcd_.setCursor(0,1);
    lcd_.print(i+1);
    lcd_.print(". ");
    lcd_.print(this->scores[maxIndex].value >> 4);
    lcd_.print(' ');
    lcd_.print(this->scores[maxIndex].name[0]);
    lcd_.print(this->scores[maxIndex].name[1]);
    lcd_.print(this->scores[maxIndex].name[2]);
    lcd_.print("         ");
    delay(1000);
  }
}

void
DeuligneHiScores::reset()
{
  for (int i = 0; i < number_; ++i)
  {
    this->scores[i].value = 0x0f - i;
    this->scores[i].name[0] = 'A';
    this->scores[i].name[1] = 'A';
    this->scores[i].name[2] = 'A';
    writeScore(i);
  }
}
