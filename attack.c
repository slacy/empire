/*
 *    Copyright (C) 1987, 1988 Chuck Simmons
 * 
 * See the file COPYING, distributed with empire, for restriction
 * and warranty information.
 */

/*
attack.c -- handle an attack between two pieces.  We do everything from
fighting it out between the pieces to notifying the user who won and
killing off the losing object.  Somewhere far above, our caller is
responsible for actually removing the object from its list and actually
updating the player's view of the world.

Find object being attacked.  If it is a city, attacker has 50% chance
of taking city.  If successful, give city to attacker.  Otherwise
kill attacking piece.  Tell user who won.

If attacking object is not a city, loop.  On each iteration, select one
piece to throw a blow.  Damage the opponent by the strength of the blow
thrower.  Stop looping when one object has 0 or fewer hits.  Kill off 
the dead object.  Tell user who won and how many hits her piece has left,
if any.
*/

#include "empire.h"
#include "extern.h"

void
attack_city(piece_info_t *att_obj, loc_t loc)
{
    city_info_t *cityp;
    int att_owner, city_owner;

    cityp = find_city (loc);
    ASSERT (cityp);
	
    att_owner = att_obj->owner;
    city_owner = cityp->owner;

    if (irand (2) == 0) { /* attack fails? */
	if (att_owner == USER) {
	    comment ("The scum defending the city crushed your attacking blitzkrieger.");
	    ksend ("The scum defending the city crushed your attacking blitzkrieger.\n"); //kermyt
	}
	else if (city_owner == USER) {
	    ksend ("Your city at %d is under attack.\n",loc_disp(cityp->loc)); //kermyt
	    comment ("Your city at %d is under attack.",loc_disp(cityp->loc));
	}
	kill_obj (att_obj, loc);
    }
    else { /* attack succeeded */
	kill_city (cityp);
	cityp->owner = att_owner;
	kill_obj (att_obj, loc);

	if (att_owner == USER) {
	    ksend ("City at %d has been subjugated!\n",loc_disp(cityp->loc)); //kermyt
	    error ("City at %d has been subjugated!",loc_disp(cityp->loc));

	    extra ("Your army has been dispersed to enforce control.");
	    ksend ("Your army has been dispersed to enforce control.\n");
	    set_prod (cityp);
	}
	else if (city_owner == USER) {
	    ksend("City at %d has been lost to the enemy!\n",loc_disp(cityp->loc)); //kermyt
	    comment ("City at %d has been lost to the enemy!",loc_disp(cityp->loc));
	}
    }
    /* let city owner see all results */
    if (city_owner != UNOWNED) scan (MAP(city_owner), loc);
}

/*
Attack a piece other than a city.  The piece could be anyone's.
First we have to figure out what is being attacked.
*/

void
attack_obj(piece_info_t *att_obj, loc_t loc)
{
    void describe(), survive();
	
    piece_info_t *def_obj; /* defender */
    int owner;

    def_obj = find_obj_at_loc (loc);
    ASSERT (def_obj != NULL); /* can't find object to attack? */
	
    if (def_obj->type == SATELLITE) return; /* can't attack a satellite */

    while (att_obj->hits > 0 && def_obj->hits > 0) {
	if (irand (2) == 0) /* defender hits? */
	    att_obj->hits -= piece_attr[def_obj->type].strength;
	else def_obj->hits -= piece_attr[att_obj->type].strength;
    }

    if (att_obj->hits > 0) { /* attacker won? */
	describe (att_obj, def_obj, loc);
	owner = def_obj->owner;
	kill_obj (def_obj, loc); /* kill loser */
	survive (att_obj, loc); /* move attacker */
    }
    else { /* defender won */
	describe (def_obj, att_obj, loc);
	owner = att_obj->owner;
	kill_obj (att_obj, loc);
	survive (def_obj, loc);
    }
    /* show results to first killed */
    scan (MAP(owner), loc);
}

void
attack(piece_info_t *att_obj, loc_t loc)
{
    if (map[loc].contents == MAP_CITY) /* attacking a city? */
	attack_city (att_obj, loc);
    else attack_obj (att_obj, loc); /* attacking a piece */
}

/*
Here we look to see if any cargo was killed in the attack.  If
a ships contents exceeds its capacity, some of the survivors
fall overboard and drown.  We also move the survivor to the given
location.
*/

void
survive(piece_info_t *obj, loc_t loc)
{
    while (obj_capacity (obj) < obj->count)
	kill_obj (obj->cargo, loc);
		
    move_obj (obj, loc);
}

void
describe(piece_info_t *win_obj, piece_info_t *lose_obj, loc_t loc)
{
    char buf[STRSIZE];
    char buf2[STRSIZE];
	
    *buf = '\0';
    *buf2 = '\0';
	
    if (win_obj->owner != lose_obj->owner) {
	if (win_obj->owner == USER) {
	    int diff;
	    user_score += piece_attr[lose_obj->type].build_time; 
	    ksend ("Enemy %s at %d destroyed.\n",piece_attr[lose_obj->type].name,loc_disp(loc)); //kermyt
	    topmsg (1, "Enemy %s at %d destroyed.",piece_attr[lose_obj->type].name,loc_disp(loc));
	    ksend ("Your %s has %d hits left\n",piece_attr[win_obj->type].name,win_obj->hits); //kermyt
	    topmsg (2, "Your %s has %d hits left.", piece_attr[win_obj->type].name, win_obj->hits);
				
	    diff = win_obj->count - obj_capacity (win_obj);
	    if (diff > 0) switch (win_obj->cargo->type) {
		case ARMY:
		    ksend("%d armies fell overboard and drowned in the assault.\n",diff); //kermyt
		    topmsg (3,"%d armies fell overboard and drowned in the assault.",diff);
		    break;
		case FIGHTER:
		    ksend("%d fighters fell overboard and were lost in the assult.\n",diff); //kermyt
		    topmsg (3,"%d fighters fell overboard and were lost in the assault.",diff);
		    break;
		}
	}
	else {
	    comp_score += piece_attr[lose_obj->type].build_time;
	    ksend ("Your %s at %d destroyed.\n",piece_attr[lose_obj->type].name,loc_disp(loc)); //kermyt
	    topmsg (3, "Your %s at %d destroyed.",piece_attr[lose_obj->type].name,loc_disp(loc));
	}
	set_need_delay ();
    }
}

/* end */
