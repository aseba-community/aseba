#ifndef __ASEBA_AUTHORS_H
#define __ASEBA_AUTHORS_H

#include <vector>
#include <set>

namespace Aseba
{
	struct AuthorInfo
	{
		using Tags = std::set<std::string>;
		
		std::string name;
		std::string email;
		std::string web;
		std::string role;
		Tags tags;
	};
	using AuthorInfos = std::vector<AuthorInfo>;
	
	struct InstitutionInfo
	{
		std::string name;
		std::string web;
	};
	using InstitutionInfos = std::vector<InstitutionInfo>;

	static const AuthorInfos authorList = {
		{
			u8"Stéphane Magnenat",
			u8"stephane@magnenat.net",
			u8"http://stephane.magnenat.net",
			u8"project leader, original idea, architecture, core programmer all areas",
			{ u8"all", }
		},
		{
			u8"Florian Vaussard",
			u8"florian.vaussard@bluewin.ch",
			u8"",
			u8"core software engineer, Windows maintainer until 2015",
			{ u8"core", u8"studio", u8"packaging", }
		},
		{
			u8"David James Sherman",
			u8"david.sherman@inria.fr",
			u8"http://www.labri.fr/perso/david/Site/David_James_Sherman.html",
			u8"core software engineer, asebahttp, Zeroconf, Scratch integration",
			{ u8"core", u8"simulator", }
		},
		{
			u8"Jiwon Shin",
			u8"jiwon@shin.ch",
			u8"",
			u8"software engineer for VPL",
			{ u8"vpl", }
		},
		{
			u8"Thibault Laine",
			u8"thibault.laine@inria.fr",
			u8"",
			u8"software engineer for Playground",
			{ u8"simulator", }
		},
		{
			u8"Philippe Rétornaz",
			u8"philippe.retornaz@a3.epfl.ch",
			u8"",
			u8"software engineer for embedded integration",
			{ u8"core", }
		},
		{
			u8"Fabian Hahn",
			u8"fabian@hahn.graphics",
			u8"",
			u8"software engineer for Blockly integration",
			{ u8"blockly", }
		},
	};
	
	static const AuthorInfos thankToList = {
		{
			u8"Maria María Beltrán",
			u8"mari_tran@yahoo.com",
			u8"",
			u8"VPL re-design - funded by Gebert Rüf Stiftung",
			{ u8"vpl", }
		},
		{
			u8"Fanny Riedo",
			u8"fanny.riedo@mobsya.org",
			u8"",
			u8"macOS packaging, translation, quality insurance",
			{ u8"packaging", u8"translation", }
		},
		{
			u8"Michael Bonani",
			u8"michael.bonani@mobsya.org",
			u8"",
			u8"French translation, packaging, quality insurance",
			{ u8"packaging", u8"translation", }
		},
		{
			u8"Dean Brettle",
			u8"dean@brettle.com",
			u8"",
			u8"RPM packaging",
			{ u8"packaging", }
		},
		{
			u8"Martin Voelkle",
			u8"martin.voelkle@sampla.ch",
			u8"",
			u8"bug reporting and fixing",
			{ u8"vpl", }
		},
		{
			u8"Yves Piguet",
			u8"",
			u8"",
			u8"bug reporting and fixing",
			{ u8"core", u8"studio", }
		},
		{
			u8"Corentin Jabot",
			u8"",
			u8"",
			u8"bug fixing and coding contributions",
			{ u8"core", }
		},
		{
			u8"Valentin Longchamp",
			u8"valentin.longchamp@gmail.com",
			u8"",
			u8"inputs on framework architecture, additional coding",
			{ u8"core", }
		},
		{
			u8"Severin Klingler",
			u8"severin.klinger@ethz.ch",
			u8"",
			u8"VPL logging",
			{ u8"vpl", }
		},
		{
			u8"Stefan Witwicki",
			u8"",
			u8"",
			u8"Examples",
			{ u8"example", }
		},
		{
			u8"Basilio Noris",
			u8"basilio.noris@gmail.com",
			u8"",
			u8"inputs on challenge, graphics design on challenge",
			{ u8"simulator", }
		},
		{
			u8"Sandra Moser ",
			u8"",
			u8"",
			u8"German translation",
			{ u8"translation", }
		},
		{
			u8"Francisco Javier Botero Herrera",
			u8"fboteroh@gmail.com",
			u8"",
			u8"Spanish translation",
			{ u8"translation", }
		},
		{
			u8"Ezio Somá",
			u8"ezio.soma@gmail.com",
			u8"",
			u8"Italian translation",
			{ u8"translation", }
		},
		{
			u8"Detlef Rick",
			u8"detlef.rick@gymnasium-hittfeld.de",
			u8"",
			u8"German translation",
			{ u8"translation", }
		},
		{
			u8"Shiling Wang",
			u8"shilingwang0621@gmail.com",
			u8"",
			u8"Chinese translation",
			{ u8"translation", }
		},
		{
			u8"Ioanna Theodoropoulou",
			u8"ioannatheodoropou@hotmail.com",
			u8"",
			u8"Greek translation",
			{ u8"translation", }
		},
		{
			u8"Vassilis Komis",
			u8"komis@upatras.gr",
			u8"",
			u8"Greek translation",
			{ u8"translation", }
		},
	};
} // namespace Aseba

#endif // __ASEBA_AUTHORS_H
