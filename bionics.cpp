#include "player.h"
#include "game.h"
#include "rng.h"
#include "keypress.h"
#include "item.h"
#include "bionics.h"
#include "line.h"

#define BATTERY_AMOUNT 4 // How much batteries increase your power

void bionics_install_failure(game *g, player *u, int success);


// Why put this in a Big Switch?  Why not let bionics have pointers to
// functions, much like monsters and items?
//
// Well, because like diseases, which are also in a Big Switch, bionics don't
// share functions....
void player::activate_bionic(int b, game *g)
{
 bionic bio = my_bionics[b];
 int power_cost = bionics[bio.id].power_cost;
 if (weapon.type->id == itm_bio_claws && bio.id == bio_claws)
  power_cost = 0;
 if (power_level < power_cost) {
  if (my_bionics[b].powered) {
   g->add_msg("Your %s powers down.", bionics[bio.id].name.c_str());
   my_bionics[b].powered = false;
  } else
   g->add_msg("You cannot power your %s", bionics[bio.id].name.c_str());
  return;
 }

 if (my_bionics[b].powered && my_bionics[b].charge > 0) {
// Already-on units just lose a bit of charge
  my_bionics[b].charge--;
 } else {
// Not-on units, or those with zero charge, have to pay the power cost
  if (bionics[bio.id].charge_time > 0) {
   my_bionics[b].powered = true;
   my_bionics[b].charge = bionics[bio.id].charge_time;
  }
  power_level -= power_cost;
 }

 std::string junk;
 std::vector<point> traj;
 std::vector<std::string> good;
 std::vector<std::string> bad;
 WINDOW* w;
 int dirx, diry, t, l, index;
 item tmp_item;

 switch (bio.id) {

 case bio_painkiller:
  pkill += 6;
  pain -= 2;
  if (pkill > pain)
   pkill = pain;
  break;

 case bio_nanobots:
  healall(4);
  break;

 case bio_resonator:
  g->sound(posx, posy, 30, "VRRRRMP!");
  for (int i = posx - 1; i <= posx + 1; i++) {
   for (int j = posy - 1; j <= posy + 1; j++) {
    g->m.bash(i, j, 40, junk);
    g->m.bash(i, j, 40, junk);	// Multibash effect, so that doors &c will fall
    g->m.bash(i, j, 40, junk);
    if (g->m.is_destructable(i, j) && rng(1, 10) >= 4)
     g->m.ter(i, j) = t_rubble;
   }
  }
  break;

 case bio_time_freeze:
  moves += 100 * power_level;
  power_level = 0;
  g->add_msg("Ваша скорость внезапно увеличилась!");
  if (one_in(3)) {
   g->add_msg("Ваши мускулы порвались от напряжения.");
   hurt(g, bp_arms, 0, rng(5, 10));
   hurt(g, bp_arms, 1, rng(5, 10));
   hurt(g, bp_legs, 0, rng(7, 12));
   hurt(g, bp_legs, 1, rng(7, 12));
   hurt(g, bp_torso, 0, rng(5, 15));
  }
  if (one_in(5))
   add_disease(DI_TELEGLOW, rng(50, 400), g);
  break;

 case bio_teleport:
  g->teleport();
  add_disease(DI_TELEGLOW, 300, g);
  break;

// TODO: More stuff here (and bio_blood_filter)
 case bio_blood_anal:
  w = newwin(20, 40, 3, 10);
  wborder(w, LINE_XOXO, LINE_XOXO, LINE_OXOX, LINE_OXOX,
             LINE_OXXO, LINE_OOXX, LINE_XXOO, LINE_XOOX );
  if (has_disease(DI_FUNGUS))
   bad.push_back("Грибковый паразит");
  if (has_disease(DI_DERMATIK))
   bad.push_back("Насекомое-паразит");
  if (has_disease(DI_POISON))
   bad.push_back("Яд");
  if (radiation > 0)
   bad.push_back("Облучен");
  if (has_disease(DI_PKILL1))
   good.push_back("Слабое болеутоляющее");
  if (has_disease(DI_PKILL2))
   good.push_back("Среднее болеутоляющее");
  if (has_disease(DI_PKILL3))
   good.push_back("Сильное болеутоляющее");
  if (has_disease(DI_PKILL_L))
   good.push_back("Медленное болеутоляющее");
  if (has_disease(DI_DRUNK))
   good.push_back("Алкоголь");
  if (has_disease(DI_CIG))
   good.push_back("Никотин");
  if (has_disease(DI_HIGH))
   good.push_back("Наркотическое воздействие");
  if (has_disease(DI_TOOK_PROZAC))
   good.push_back("Прозак");
  if (has_disease(DI_TOOK_FLUMED))
   good.push_back("Антигистамины");
  if (has_disease(DI_ADRENALINE))
   good.push_back("Выброс адреналина");
  if (good.size() == 0 && bad.size() == 0)
   mvwprintz(w, 1, 1, c_white, "Нет эффектов.");
  else {
   for (int line = 1; line < 39 && line <= good.size() + bad.size(); line++) {
    if (line <= bad.size())
     mvwprintz(w, line, 1, c_red, bad[line - 1].c_str());
    else
     mvwprintz(w, line, 1, c_green, good[line - 1 - bad.size()].c_str());
   }
  }
  wrefresh(w);
  refresh();
  getch();
  delwin(w);
  break;

 case bio_blood_filter:
  rem_disease(DI_FUNGUS);
  rem_disease(DI_POISON);
  rem_disease(DI_PKILL1);
  rem_disease(DI_PKILL2);
  rem_disease(DI_PKILL3);
  rem_disease(DI_PKILL_L);
  rem_disease(DI_DRUNK);
  rem_disease(DI_CIG);
  rem_disease(DI_HIGH);
  rem_disease(DI_TOOK_PROZAC);
  rem_disease(DI_TOOK_FLUMED);
  rem_disease(DI_ADRENALINE);
  break;

 case bio_evap:
  if (query_yn("Выпить сейчас? Иначе Вам потребуется тара.")) {
   tmp_item = item(g->itypes[itm_water], 0);
   thirst -= 50;
   if (has_trait(PF_GOURMAND) && thirst < -60) {
     g->add_msg("Вы не можете допить всё!");
     thirst = -60;
   } else if (!has_trait(PF_GOURMAND) && thirst < -20) {
     g->add_msg("Вы не можете допить всё!");
     thirst = -20;
   }
  } else {
   t = g->inv("Выберите тару:");
   if (i_at(t).type == 0) {
    g->add_msg("У Вас нет этого предмета!");
    power_level += bionics[bio_evap].power_cost;
   } else if (!i_at(t).is_container()) {
    g->add_msg("%s не тара!", i_at(t).tname().c_str());
    power_level += bionics[bio_evap].power_cost;
   } else {
    it_container *cont = dynamic_cast<it_container*>(i_at(t).type);
    if (i_at(t).volume_contained() + 1 > cont->contains) {
     g->add_msg("%s уже полная.", i_at(t).tname().c_str());
     power_level += bionics[bio_evap].power_cost;
    } else if (!(cont->flags & con_wtight)) {
     g->add_msg("%s не водонепроницаема!", i_at(t).tname().c_str());
     power_level += bionics[bio_evap].power_cost;
    } else {
     g->add_msg("%s наполнена водой.", i_at(t).tname().c_str());
     i_at(t).put_in(item(g->itypes[itm_water], 0));
    }
   }
  }
  break;

 case bio_lighter:
  g->draw();
  mvprintw(0, 0, "Где поджечь?");
  get_direction(dirx, diry, input());
  if (dirx == -2) {
   g->add_msg("Неверное направление.");
   power_level += bionics[bio_lighter].power_cost;
   return;
  }
  dirx += posx;
  diry += posy;
  if (!g->m.add_field(g, dirx, diry, fd_fire, 1))	// Unsuccessful.
   g->add_msg("Вы не можете разжесь огонь здесь.");
  break;

 case bio_claws:
  if (weapon.type->id == itm_bio_claws) {
   g->add_msg("Вы выпустили ваши когти.");
   weapon = ret_null;
  } else if (weapon.type->id != 0) {
   g->add_msg("Выпущенные когти заставили Вас выбросить Ваш %s.",
              weapon.tname().c_str());
   g->m.add_item(posx, posy, weapon);
   weapon = item(g->itypes[itm_bio_claws], 0);
   weapon.invlet = '#';
  } else {
   g->add_msg("Выши когти выпущены!");
   weapon = item(g->itypes[itm_bio_claws], 0);
   weapon.invlet = '#';
  }
  break;

 case bio_blaster:
  tmp_item = weapon;
  weapon = item(g->itypes[itm_bio_blaster], 0);
  weapon.curammo = dynamic_cast<it_ammo*>(g->itypes[itm_bio_fusion]);
  weapon.charges = 1;
  g->refresh_all();
  g->plfire(false);
  weapon = tmp_item;
  break;

 case bio_laser:
  tmp_item = weapon;
  weapon = item(g->itypes[itm_v29], 0);
  weapon.curammo = dynamic_cast<it_ammo*>(g->itypes[itm_laser_pack]);
  weapon.charges = 1;
  g->refresh_all();
  g->plfire(false);
  weapon = tmp_item;
  break;

 case bio_emp:
  g->draw();
  mvprintw(0, 0, "Куда послать ЭМИ?");
  get_direction(dirx, diry, input());
  if (dirx == -2) {
   g->add_msg("Неверное направление.");
   power_level += bionics[bio_emp].power_cost;
   return;
  }
  dirx += posx;
  diry += posy;
  g->emp_blast(dirx, diry);
  break;

 case bio_hydraulics:
  g->add_msg("Гидравлические приводы с шипением наполняют Ваши мускулы силой!");
  break;

 case bio_water_extractor:
  for (int i = 0; i < g->m.i_at(posx, posy).size(); i++) {
   item tmp = g->m.i_at(posx, posy)[i];
   if (tmp.type->id == itm_corpse && query_yn("Вылить воду из %s",
                                              tmp.tname().c_str())) {
    i = g->m.i_at(posx, posy).size() + 1;	// Loop is finished
    t = g->inv("Выберите тару:");
    if (i_at(t).type == 0) {
     g->add_msg("У Вас нет этого предмета!");
     power_level += bionics[bio_water_extractor].power_cost;
    } else if (!i_at(t).is_container()) {
     g->add_msg("%s не тара!", i_at(t).tname().c_str());
     power_level += bionics[bio_water_extractor].power_cost;
    } else {
     it_container *cont = dynamic_cast<it_container*>(i_at(t).type);
     if (i_at(t).volume_contained() + 1 > cont->contains) {
      g->add_msg("%s уже полная.", i_at(t).tname().c_str());
      power_level += bionics[bio_water_extractor].power_cost;
     } else {
      g->add_msg("%s наполнена водой.", i_at(t).tname().c_str());
      i_at(t).put_in(item(g->itypes[itm_water], 0));
     }
    }
   }
   if (i == g->m.i_at(posx, posy).size() - 1)	// We never chose a corpse
    power_level += bionics[bio_water_extractor].power_cost;
  }
  break;

 case bio_magnet:
  for (int i = posx - 10; i <= posx + 10; i++) {
   for (int j = posy - 10; j <= posy + 10; j++) {
    if (g->m.i_at(i, j).size() > 0) {
     if (g->m.sees(i, j, posx, posy, -1, t))
      traj = line_to(i, j, posx, posy, t);
     else
      traj = line_to(i, j, posx, posy, 0);
    }
    traj.insert(traj.begin(), point(i, j));
    for (int k = 0; k < g->m.i_at(i, j).size(); k++) {
     if (g->m.i_at(i, j)[k].made_of(IRON) || g->m.i_at(i, j)[k].made_of(STEEL)){
      tmp_item = g->m.i_at(i, j)[k];
      g->m.i_rem(i, j, k);
      for (l = 0; l < traj.size(); l++) {
       index = g->mon_at(traj[l].x, traj[l].y);
       if (index != -1) {
        if (g->z[index].hurt(tmp_item.weight() * 2))
         g->kill_mon(index);
        g->m.add_item(traj[l].x, traj[l].y, tmp_item);
        l = traj.size() + 1;
       } else if (l > 0 && g->m.move_cost(traj[l].x, traj[l].y) == 0) {
        g->m.bash(traj[l].x, traj[l].y, tmp_item.weight() * 2, junk);
        g->sound(traj[l].x, traj[l].y, 12, junk);
        if (g->m.move_cost(traj[l].x, traj[l].y) == 0) {
         g->m.add_item(traj[l - 1].x, traj[l - 1].y, tmp_item);
         l = traj.size() + 1;
        }
       }
      }
      if (l == traj.size())
       g->m.add_item(posx, posy, tmp_item);
     }
    }
   }
  }
  break;

 case bio_lockpick:
  g->draw();
  mvprintw(0, 0, "Что хотите вскрыть?");
  get_direction(dirx, diry, input());
  if (dirx == -2) {
   g->add_msg("Неверное направление.");
   power_level += bionics[bio_lockpick].power_cost;
   return;
  }
  dirx += posx;
  diry += posy;
  if (g->m.ter(dirx, diry) == t_door_locked) {
   moves -= 40;
   g->add_msg("Вы вскрыли дверь.");
   g->m.ter(dirx, diry) = t_door_c;
  } else
   g->add_msg("%s Вам не поддается.", g->m.tername(dirx, diry).c_str());
  break;

 }
}

bool player::install_bionics(game *g, it_bionic* type)
{
 if (type == NULL) {
  debugmsg("Tried to install NULL bionic");
  return false;
 }
 std::string bio_name = type->name.substr(5);	// Strip off "CBM: "
 WINDOW* w = newwin(25, 80, 0, 0);

 int pl_skill = int_cur + sklevel[sk_electronics] * 4 +
                          sklevel[sk_firstaid]    * 3 +
                          sklevel[sk_mechanics]   * 2;

 int skint = int(pl_skill / 4);
 int skdec = int((pl_skill * 10) / 4) % 10;

// Header text
 mvwprintz(w, 0,  0, c_white, "Установка имплантов:");
 mvwprintz(w, 0, 20, type->color, bio_name.c_str());

// Dividing bars
 for (int i = 0; i < 80; i++) {
  mvwputch(w,  1, i, c_ltgray, LINE_OXOX);
  mvwputch(w, 21, i, c_ltgray, LINE_OXOX);
 }
// Init the list of bionics
 for (int i = 1; i < type->options.size(); i++) {
  bionic_id id = type->options[i];
  mvwprintz(w, i + 2, 0, (has_bionic(id) ? c_ltred : c_ltblue),
            bionics[id].name.c_str());
 }
// Helper text
 mvwprintz(w, 2, 40, c_white,        "Сложность этого модуля: %d",
           type->difficulty);
 mvwprintz(w, 3, 40, c_white,        "Вы способны установить:   %d.%d",
           skint, skdec);
 mvwprintz(w, 4, 40, c_white,       "Установка требует большого ума,");
 mvwprintz(w, 5, 40, c_white,       "и знаний по электронике, первой помощи, и");
 mvwprintz(w, 6, 40, c_white,       "механике (в порядке их важности).");

 int chance_of_success = int((100 * pl_skill) /
                             (pl_skill + 4 * type->difficulty));

 mvwprintz(w, 8, 40, c_white,        "Шанс на успех:");

 nc_color col_suc;
 if (chance_of_success >= 95)
  col_suc = c_green;
 else if (chance_of_success >= 80)
  col_suc = c_ltgreen;
 else if (chance_of_success >= 60)
  col_suc = c_yellow;
 else if (chance_of_success >= 35)
  col_suc = c_ltred;
 else
  col_suc = c_red;

 mvwprintz(w, 8, 59, col_suc, "%d%%%%", chance_of_success);

 mvwprintz(w, 10, 40, c_white,       "Неудача может окончиться травмами,");
 mvwprintz(w, 11, 40, c_white,       "потерей установленных имплантов, мутацией");
 mvwprintz(w, 12, 40, c_white,       "или незавершенной установкой.");
 wrefresh(w);

 if (type->id == itm_bionics_battery) {	// No selection list; just confirm
  mvwprintz(w,  2, 0, h_ltblue, "Заряд энергии +%d", BATTERY_AMOUNT);
  mvwprintz(w, 22, 0, c_ltblue, "\
Установка этого импланта увеличит заряд Вашей энергии на 10.\n\
Энергия необходима для работы многих имплантов.  Они также требуют\n\
заряжающее устройство, которое может быть установлено другим КБМ.");
  char ch;
  wrefresh(w);
  do
   ch = getch();
  while (ch != 'q' && ch != '\n' && ch != KEY_ESCAPE);
  if (ch == '\n') {
   int success = chance_of_success - rng(1, 100);
   if (success > 0) {
    g->add_msg("Заряд увеличен.");
    max_power_level += BATTERY_AMOUNT;
   } else
    bionics_install_failure(g, this, success);
   werase(w);
   delwin(w);
   g->refresh_all();
   return true;
  }
  werase(w);
  delwin(w);
  g->refresh_all();
  return false;
 }

 int selection = 0;
 char ch;

 do {

  bionic_id id = type->options[selection];
  mvwprintz(w, 2 + selection, 0, (has_bionic(id) ? h_ltred : h_ltblue),
            bionics[id].name.c_str());

// Clear the bottom three lines...
  mvwprintz(w, 22, 0, c_ltgray, "\
                                                                             \n\
                                                                             \n\
                                                                             ");
// ...and then fill them with the description of the selected bionic
  mvwprintz(w, 22, 0, c_ltblue, bionics[id].description.c_str());

  wrefresh(w);
  ch = input();
  switch (ch) {

  case 'j':
   mvwprintz(w, 2 + selection, 0, (has_bionic(id) ? c_ltred : c_ltblue),
             bionics[id].name.c_str());
   if (selection == type->options.size() - 1)
    selection = 0;
   else
    selection++;
   break;

  case 'k':
   mvwprintz(w, 2 + selection, 0, (has_bionic(id) ? c_ltred : c_ltblue),
             bionics[id].name.c_str());
   if (selection == 0)
    selection = type->options.size() - 1;
   else
    selection--;
   break;

  }
  if (ch == '\n' && has_bionic(id)) {
   popup("У Вас уже установлен %s!", bionics[id].name.c_str());
   ch = 'a';
  }
 } while (ch != '\n' && ch != 'q' && ch != KEY_ESCAPE);

 if (ch == '\n') {
  bionic_id id = type->options[selection];
  int success = chance_of_success - rng(1, 100);
  if (success > 0) {
   g->add_msg("Успешно установлен %s.", bionics[id].name.c_str());
   add_bionic(id);
  } else
   bionics_install_failure(g, this, success);
  werase(w);
  delwin(w);
  g->refresh_all();
  return true;
 }
 werase(w);
 delwin(w);
 g->refresh_all();
 return false;
}

void bionics_install_failure(game *g, player *u, int success)
{
 success = abs(success) - rng(1, 10);
 int failure_level = 0;
 if (success <= 0) {
  g->add_msg("Установка прервана без последствий.");
  return;
 }

 while (success > 0) {
  failure_level++;
  success -= rng(1, 10);
 }

 int fail_type = rng(1, (failure_level > 5 ? 5 : failure_level));
 std::string fail_text;

 switch (rng(1, 5)) {
  case 1: fail_text = "Вы запороли процесс установки";	break;
  case 2: fail_text = "Вы нарушили процесс установки";	break;
  case 3: fail_text = "Установка прервана";		break;
  case 4: fail_text = "Ошибка в процессе установки";	break;
  case 5: fail_text = "Вы напортачили в процессе установки";	break;
 }

 if (fail_type == 3 && u->my_bionics.size() == 0)
  fail_type = 2; // If we have no bionics, take damage instead of losing some

 switch (fail_type) {

 case 1:
  fail_text += ", добившись адской боли.";
  u->pain += rng(failure_level * 3, failure_level * 6);
  break;
 
 case 2:
  fail_text += " Ваше тело травмировано.";
  u->hurtall(rng(failure_level, failure_level * 2));
  break;

 case 3:
  fail_text += " и ";
  fail_text += (u->my_bionics.size() <= failure_level ? "все" : "некоторые");
  fail_text += " установленные импланты испорчены.";
  for (int i = 0; i < failure_level && u->my_bionics.size() > 0; i++) {
   int rem = rng(0, u->my_bionics.size() - 1);
   u->my_bionics.erase(u->my_bionics.begin() + rem);
  }
  break;

 case 4:
  fail_text += " испортили свой генокод, мутировав.";
  g->add_msg(fail_text.c_str()); // Failure text comes BEFORE mutation text
  while (failure_level > 0) {
   u->mutate(g);
   failure_level -= rng(1, failure_level + 2);
  }
  return;	// So the failure text doesn't show up twice
  break;

 case 5:
 {
  fail_text += ", устанавливаемый имплант сломан.";
  std::vector<bionic_id> valid;
  for (int i = max_bio_good + 1; i < max_bio; i++) {
   bionic_id id = bionic_id(i);
   if (!u->has_bionic(id))
    valid.push_back(id);
  }
  if (valid.size() == 0) {	// We've got all the bad bionics!
   if (u->max_power_level > 0) {
    g->add_msg("Вы теряете заряд энергии!");
    u->max_power_level = rng(0, u->max_power_level - 1);
   }
// TODO: What if we can't lose power capacity?  No penalty?
  } else {
   int index = rng(0, valid.size() - 1);
   u->add_bionic(valid[index]);
  }
 }
  break;
 }

 g->add_msg(fail_text.c_str());

}

