#ifndef CONFIG_H
#define CONFIG_H

void load_config(int *volume, int *auto_next, int *shuffle, int *loop, int *sort_mode);
void save_config(int volume, int auto_next, int shuffle, int loop, int sort_mode);

#endif
