# AUDIT: batting_physics.c

## Function 1: calculate_pitch_frame_time()

### Original Code (cf7780a):
```c
// Line ~line 308 in batting_system.c
float v = velocity_y;
float a = -GRAVITY;
float s = 0.0f;

pitchFrameTime = (int)((-v - sqrt(v*v - 2*a*s))/a) + PITCH_FRAME_TIME_TWEAK;
```

### Extracted Pure Function:
```c
int calculate_pitch_frame_time(float velocity_y, float gravity, float start_height, int tweak_frames)
{
    float v = velocity_y;
    float a = -gravity;
    float s = start_height;
    
    if (fabs(a) < 0.000001f) return 0;
    
    float discriminant = v*v - 2*a*s;
    if (discriminant < 0) return 0;
    
    float t = (-v - sqrtf(discriminant)) / a;
    return (int)t + tweak_frames;
}
```

### Current Call Site:
```c
pitchFrameTime = calculate_pitch_frame_time(v, GRAVITY, 0.0f, PITCH_FRAME_TIME_TWEAK);
```

### Parameter Mapping:
| Original | Extracted Param | Call Site Value | Match? |
|----------|----------------|-----------------|--------|
| velocity_y (v) | velocity_y | v | ✅ YES |
| GRAVITY | gravity | GRAVITY | ✅ YES |
| 0.0f (hardcoded s) | start_height | 0.0f | ✅ YES |
| PITCH_FRAME_TIME_TWEAK | tweak_frames | PITCH_FRAME_TIME_TWEAK | ✅ YES |

### Formula Check:
**Original:** `(int)((-v - sqrt(v*v - 2*a*s))/a) + TWEAK`
**Extracted:** `(int)((-v - sqrtf(discriminant))/a) + tweak` where `discriminant = v*v - 2*a*s`

✅ **IDENTICAL LOGIC**

### Safety Additions:
- ✅ Added divide-by-zero check for gravity
- ✅ Added negative discriminant check
- ✅ Changed `sqrt()` to `sqrtf()` (float optimized)

### Verdict: ✅ **CORRECT**
No bugs found. The extraction is faithful and even adds safety checks.

---
## Function 2: calculate_batting_vertical_angle()

### Original Code (cf7780a):
```c
scaleNumber = (float)(selectedBattingPowerCount + (BAT_SWING_MAX - BAT_LOAD_MAX));
zeroNumber = BAT_SWING_MAX*(1.0f*selectedBattingPowerCount / scaleNumber);
verticalAngle = 7 * stateInfo->localGameInfo->ballInfo.velocity.y * 
                (selectedBattingAngleCount - zeroNumber) * (scaleNumber / BAT_SWING_MAX);
```

### Extracted Pure Function:
```c
float calculate_batting_vertical_angle(int power_count, int angle_count, float ball_vy, 
                                       int swing_max, int load_max)
{
    float scaleNumber = (float)(power_count + (swing_max - load_max));
    if (fabs(scaleNumber) < 0.00001f) return 0.0f;
    
    float zeroNumber = swing_max * (1.0f * power_count / scaleNumber);
    
    float verticalAngle = 7.0f * ball_vy * (angle_count - zeroNumber) * (scaleNumber / swing_max);
    
    return verticalAngle;
}
```

### Call Site Search Needed:
Let me find where this is called...

### Current Call Site:
```c
verticalAngle = calculate_batting_vertical_angle(selectedBattingPowerCount, 
                                                  selectedBattingAngleCount, 
                                                  stateInfo->localGameInfo->ballInfo.velocity.y, 
                                                  BAT_SWING_MAX, 
                                                  BAT_LOAD_MAX);
```

### Parameter Mapping:
| Original Variable | Extracted Param | Call Site Value | Match? |
|----------|----------------|-----------------|--------|
| selectedBattingPowerCount | power_count | selectedBattingPowerCount | ✅ YES |
| selectedBattingAngleCount | angle_count | selectedBattingAngleCount | ✅ YES |
| ballInfo.velocity.y | ball_vy | ballInfo.velocity.y | ✅ YES |
| BAT_SWING_MAX | swing_max | BAT_SWING_MAX | ✅ YES |
| BAT_LOAD_MAX | load_max | BAT_LOAD_MAX | ✅ YES |

### Formula Check:
**Original:**
```
scaleNumber = powerCount + (swing_max - load_max)
zeroNumber = swing_max * (1.0 * powerCount / scaleNumber)
result = 7 * ball_vy * (angleCount - zeroNumber) * (scaleNumber / swing_max)
```

**Extracted:** IDENTICAL ✅

### Safety Additions:
- ✅ Added divide-by-zero check for scaleNumber

### Verdict: ✅ **CORRECT**
No bugs found. Perfect extraction with added safety.

---
## Function 3 & 4: calculate_power_meter_value() & calculate_angle_meter_value()

### Original Power Meter:
```c
stateInfo->localGameInfo->pRAI.swingMeterValue = 1.0f*meterCounter / meterCounterMax;
```

### Extracted Pure Function:
```c
float calculate_power_meter_value(int counter, int max)
{
    if (max == 0) return 0.0f;
    return 1.0f * counter / max;
}
```

### Call Site:
```c
stateInfo->localGameInfo->pRAI.swingMeterValue = calculate_power_meter_value(meterCounter, meterCounterMax);
```

✅ **CORRECT** - Direct extraction with added divide-by-zero check.

---

### Original Angle Meter:
```c
upperLimit = (float)(selectedBattingPowerCount + (BAT_SWING_MAX - BAT_LOAD_MAX)) / BAT_SWING_MAX;
lowerLimit = 0.0f;
stateInfo->localGameInfo->pRAI.swingMeterValue = upperLimit - (1.0f*meterCounter / meterCounterMax) *
    (upperLimit - lowerLimit);
```

### Extracted Pure Function:
```c
float calculate_angle_meter_value(int counter, int max, int power_count, int swing_max, int load_max)
{
    if (max == 0) return 0.0f;
    
    float upperLimit = (float)(power_count + (swing_max - load_max)) / swing_max;
    float lowerLimit = 0.0f;
    
    return upperLimit - (1.0f * counter / max) * (upperLimit - lowerLimit);
}
```

### Call Site:
```c
stateInfo->localGameInfo->pRAI.swingMeterValue = calculate_angle_meter_value(meterCounter, meterCounterMax, 
                                                                              selectedBattingPowerCount, 
                                                                              BAT_SWING_MAX, BAT_LOAD_MAX);
```

✅ **CORRECT** - Perfect extraction with added safety check.

---

# SUMMARY: batting_physics.c

| Function | Status | Notes |
|----------|--------|-------|
| calculate_pitch_frame_time() | ✅ CORRECT | Added safety checks |
| calculate_batting_vertical_angle() | ✅ CORRECT | Added div-by-zero check |
| calculate_power_meter_value() | ✅ CORRECT | Added div-by-zero check |
| calculate_angle_meter_value() | ✅ CORRECT | Added safety check |

**Overall Verdict: ✅ ALL CORRECT - NO BUGS FOUND**

All parameters match, all formulas identical, added safety improvements.
