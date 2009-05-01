/*
	Challenge - Virtual Robot Challenge System
	Copyright (C) 1999 - 2008:
		Stephane Magnenat <stephane at magnenat dot net>
		(http://stephane.magnenat.net)
	3D models
	Copyright (C) 2008:
		Basilio Noris
	Aseba - an event-based framework for distributed robot control
	Copyright (C) 2006 - 2008:
		Stephane Magnenat <stephane at magnenat dot net>
		(http://stephane.magnenat.net)
		Mobots group (http://mobots.epfl.ch)
		Laboratory of Robotics Systems (http://lsro.epfl.ch)
		EPFL, Lausanne (http://www.epfl.ch)
	
	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	any other version as decided by the two original authors
	Stephane Magnenat and Valentin Longchamp.
	
	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.
	
	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "../../vm/natives.h"

#define SIMPLIFIED_EPUCK

AsebaVMDescription vmDescription_en = {
	"e-puck",
	{
		{ 1, "wheel_left_speed" },
		{ 1, "wheel_right_speed" },
		{ 1, "color_red" },
		{ 1, "color_green" },
		{ 1, "color_blue" },
		#ifdef SIMPLIFIED_EPUCK
		{ 1, "prox_front_front_right" },
		{ 1, "prox_front_right" },
		{ 1, "prox_right" },
		{ 1, "prox_back_right" },
		{ 1, "prox_back_left" },
		{ 1, "prox_left" },
		{ 1, "prox_front_left" },
		{ 1, "prox_front_front_left" },
		{ 8, "prox" },
		#else
		{ 8, "prox" },
		#endif
		#ifdef SIMPLIFIED_EPUCK
		{ 1, "cam_red_left" },
		{ 1, "cam_red_middle" },
		{ 1, "cam_red_right" },
		{ 3, "cam_red" },
		#else
		{ 60, "camR" },
		#endif
		#ifdef SIMPLIFIED_EPUCK
		{ 1, "cam_green_left" },
		{ 1, "cam_green_middle" },
		{ 1, "cam_green_right" },
		{ 3, "cam_green" },
		#else
		{ 60, "camG" },
		#endif
		#ifdef SIMPLIFIED_EPUCK
		{ 1, "cam_blue_left" },
		{ 1, "cam_blue_middle" },
		{ 1, "cam_blue_right" },
		{ 3, "cam_blue" },
		#else
		{ 60, "camB" },
		#endif
		{ 1, "energy" },
		{ 0, NULL }
	}
};

AsebaVMDescription vmDescription_fr = {
	"e-puck",
	{
		{ 1, "roues_vitesse_gauche" },
		{ 1, "roues_vitesse_droite" },
		{ 1, "couleur_rouge" },
		{ 1, "couleur_vert" },
		{ 1, "couleur_bleu" },
		#ifdef SIMPLIFIED_EPUCK
		{ 1, "prox_avant_avant_droite" },
		{ 1, "prox_avant_droite" },
		{ 1, "prox_droite" },
		{ 1, "prox_arriere_droite" },
		{ 1, "prox_arriere_gauche" },
		{ 1, "prox_gauche" },
		{ 1, "prox_avant_gauche" },
		{ 1, "prox_avant_avant_gauche" },
		{ 8, "prox" },
		#else
		{ 8, "prox" },
		#endif
		#ifdef SIMPLIFIED_EPUCK
		{ 1, "cam_rouge_gauche" },
		{ 1, "cam_rouge_milieu" },
		{ 1, "cam_rouge_droite" },
		{ 3, "cam_rouge" },
		#else
		{ 60, "camR" },
		#endif
		#ifdef SIMPLIFIED_EPUCK
		{ 1, "cam_vert_gauche" },
		{ 1, "cam_vert_milieu" },
		{ 1, "cam_vert_droite" },
		{ 3, "cam_vert" },
		#else
		{ 60, "camG" },
		#endif
		#ifdef SIMPLIFIED_EPUCK
		{ 1, "cam_bleu_gauche" },
		{ 1, "cam_bleu_milieu" },
		{ 1, "cam_bleu_droite" },
		{ 3, "cam_bleu" },
		#else
		{ 60, "camB" },
		#endif
		{ 1, "energie" },
		{ 0, NULL }
	}
};




