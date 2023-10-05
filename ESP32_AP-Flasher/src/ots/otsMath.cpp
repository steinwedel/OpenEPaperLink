#include "ots/otsMath.h"
#include <cmath>
#include "ots/otsDebug.h"
#include <String>

void otsMathAdd(DynamicJsonDocument &vars) {
    int var1 = vars["var1"].as<int>();
    int var2 = vars["var2"].as<int>();
    otsDEBUG("var1="+String(var1)+" var2="+String(var2))
    vars[vars["resultKey"].as<String>()] = var1 + var2;
}

void otsMathDivide(DynamicJsonDocument &vars) {
    int var1 = vars["var1"].as<int>();
    int var2 = vars["var2"].as<int>();
    vars[vars["resultKey"].as<String>()] = var1 / var2;
}

void otsMathIsEqual(DynamicJsonDocument &vars) {
    int var1 = vars["var1"].as<int>();
    int var2 = vars["var2"].as<int>();
    if (var1 == var2) {
        vars[vars["resultKey"].as<String>()] = vars["resultTrue"];
    } else {
        vars[vars["resultKey"].as<String>()] = vars["resultFalse"];
    }
}

void otsMathIsEqualOrGreater(DynamicJsonDocument &vars) {
    int var1 = vars["var1"].as<int>();
    int var2 = vars["var2"].as<int>();
    if (var1 >= var2) {
        vars[vars["resultKey"].as<String>()] = vars["resultTrue"];
    } else {
        vars[vars["resultKey"].as<String>()] = vars["resultFalse"];
    }
}

void otsMathIsEqualOrSmaller(DynamicJsonDocument &vars) {
    int var1 = vars["var1"].as<int>();
    int var2 = vars["var2"].as<int>();
    if (var1 <= var2) {
        vars[vars["resultKey"].as<String>()] = vars["resultTrue"];
    } else {
        vars[vars["resultKey"].as<String>()] = vars["resultFalse"];
    }
}

void otsMathIsGreater(DynamicJsonDocument &vars) {
    int var1 = vars["var1"].as<int>();
    int var2 = vars["var2"].as<int>();
    if (var1 > var2) {
        vars[vars["resultKey"].as<String>()] = vars["resultTrue"];
    } else {
        vars[vars["resultKey"].as<String>()] = vars["resultFalse"];
    }
}

void otsMathIsSmaller(DynamicJsonDocument &vars) {
    int var1 = vars["var1"].as<int>();
    int var2 = vars["var2"].as<int>();
    if (var1 < var2) {
        vars[vars["resultKey"].as<String>()] = vars["resultTrue"];
    } else {
        vars[vars["resultKey"].as<String>()] = vars["resultFalse"];
    }
}

void otsMathModulo(DynamicJsonDocument &vars) {
    int var1 = vars["var1"].as<int>();
    int var2 = vars["var2"].as<int>();
    vars[vars["resultKey"].as<String>()] = var1 % var2;
}

void otsMathMultiply(DynamicJsonDocument &vars) {
    int var1 = vars["var1"].as<int>();
    int var2 = vars["var2"].as<int>();
    vars[vars["resultKey"].as<String>()] = var1 * var2;
}

void otsMathPowerOf(DynamicJsonDocument &vars) {
    int var1 = vars["var1"].as<int>();
    int var2 = vars["var2"].as<int>();
    vars[vars["resultKey"].as<String>()] = pow(var1, var2);
}


void otsMathSubtract(DynamicJsonDocument &vars) {
    int var1 = vars["var1"].as<int>();
    int var2 = vars["var2"].as<int>();
    vars[vars["resultKey"].as<String>()] = var1 - var2;
}