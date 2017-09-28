/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*
 * Storage.h --- AP_HAL_REVOMINI storage driver.
 *
 * Copyright (C) 2013, Virtualrobotix.com Roberto Navoni , Emile 
 * All Rights Reserved.
 *
 * This software is released under the "BSD3" license.  Read the file
 * "LICENSE" for more information.
 *
 * Written by Roberto Navoni  <info@virtualrobotix.com>, 11 January 2013
 */


// another way - https://habrahabr.ru/post/262163/

#ifndef __AP_HAL_REVOMINI_STORAGE_H__
#define __AP_HAL_REVOMINI_STORAGE_H__

#include <AP_HAL_REVOMINI/AP_HAL_REVOMINI.h>
#include "AP_HAL_REVOMINI_Namespace.h"
#include <hal.h>

class REVOMINI::REVOMINIStorage : public AP_HAL::Storage
{
public:
  REVOMINIStorage();
  void init();
  void     read_block(void *dst, uint16_t src, size_t n);
  void write_block(uint16_t dst, const void* src, size_t n);

// just for us
  uint8_t  read_byte(uint16_t loc);

  void write_byte(uint16_t loc, uint8_t value);
//  void format_eeprom(void);
};

#endif // __AP_HAL_REVOMINI_STORAGE_H__
