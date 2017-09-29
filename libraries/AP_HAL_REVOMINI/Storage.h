/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*
 * Storage.h --- AP_HAL_REVOMINI storage driver.
 
 based on:
 
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
  static void late_init(bool defer);

  void     read_block(void *dst, uint16_t src, size_t n);
  void write_block(uint16_t dst, const void* src, size_t n);

// just for us
  uint8_t  read_byte(uint16_t loc);
  uint8_t  _read_byte(uint16_t loc);

  void write_byte(uint16_t loc, uint8_t value);
  void _write_byte(uint16_t loc, uint8_t value);
//  void format_eeprom(void);
private:
     static void do_on_disarm();
    static bool write_deferred;
    
    static void error_parse(uint16_t status);
};

#endif // __AP_HAL_REVOMINI_STORAGE_H__
