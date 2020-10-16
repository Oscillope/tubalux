#ifndef UI_H
#define UI_H
#include <stdint.h>

typedef struct ui_menu ui_menu_t;

struct ui_menu {
	char name[17]; /* Must be first! */
	int8_t selection;
	uint8_t submenu_num;
	ui_menu_t* submenu;
};

void ui_init(void);
#endif /* UI_H */
