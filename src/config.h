
#define BIG_NUMBERS       false

#define KEY_TEMPERATURE   0
#define KEY_POP           1

#define RANGE_RED         40
#define RANGE_GREEN       100
#define RANGE_BLUE        100


#define BT_Y_OFFSET       140

#define HOUR_Y_OFFSET     BIG_NUMBERS ? PBL_IF_ROUND_ELSE (42,27) : PBL_IF_ROUND_ELSE (61,56)
#define HOUR_X_OFFSET     BIG_NUMBERS ? 3 : 0
#define HOUR_HEIGHT       BIG_NUMBERS ? 75 : 50

#define DATE_Y_OFFSET     PBL_IF_ROUND_ELSE (BIG_NUMBERS ? 123 : 113, 108)

#define BAR_BEGINNING     PBL_IF_ROUND_ELSE (BIG_NUMBERS ? 120 : 110, 105)
#define BAR_END           PBL_IF_ROUND_ELSE (BIG_NUMBERS ? 125 : 115, 110)
