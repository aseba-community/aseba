/*
	Challenge - Virtual Robot Challenge System
	Copyright (C) 1999--2008:
		Stephane Magnenat <stephane at magnenat dot net>
		(http://stephane.magnenat.net)
	3D models
	Copyright (C) 2008:
		Basilio Noris
	Aseba - an event-based framework for distributed robot control
	Copyright (C) 2007--2010:
		Stephane Magnenat <stephane at magnenat dot net>
		(http://stephane.magnenat.net)
		and other contributors, see authors.txt for details
	
	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU Lesser General Public License as published
	by the Free Software Foundation, version 3 of the License.
	
	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU Lesser General Public License for more details.
	
	You should have received a copy of the GNU Lesser General Public License
	along with this program. If not, see <http://www.gnu.org/licenses/>.
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
		{ 1, "dist_front_front_right" },
		{ 1, "dist_front_right" },
		{ 1, "dist_right" },
		{ 1, "dist_back_right" },
		{ 1, "dist_back_left" },
		{ 1, "dist_left" },
		{ 1, "dist_front_left" },
		{ 1, "dist_front_front_left" },
		{ 8, "dist" },
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
		{ 1, "dist_avant_avant_droite" },
		{ 1, "dist_avant_droite" },
		{ 1, "dist_droite" },
		{ 1, "dist_arriere_droite" },
		{ 1, "dist_arriere_gauche" },
		{ 1, "dist_gauche" },
		{ 1, "dist_avant_gauche" },
		{ 1, "dist_avant_avant_gauche" },
		{ 8, "dist" },
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




