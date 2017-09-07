#pragma once

#include <hal.h>

template<typename T> void ZeroIt(T& value)
{
    memset(&value,0,sizeof(value));
}

#pragma pack(push, 1)
union Revo_handler { // кровь кишки ассемблер :) преобразование функторов в унифицированный вид
    voidFuncPtr vp;
    revo_isr_handler isr;
    AP_HAL::Proc hp;
    AP_HAL::MemberProc mp;  // это должно быть доступно в С а не только С++ поэтому мы не можем объявить поддержку функторов явно, и вынуждены передавать
    Handler h;          // treat as handle             <-- как 64-битное число
    uint32_t w[2]; // words, to check. если функтор то старшее - адрес флеша, младшее - адрес в RAM. 
    //                                  Если ссылка на функцию то младшее - адрес флеша, старшее 0
};
#pragma pack(pop)
