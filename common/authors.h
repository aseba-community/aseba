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
			u8"stephane at magnenat dot net",
			u8"http://stephane.magnenat.net",
			u8"project leader, original idea, architecture, core programmer all areas",
			{ u8"all", }
		},
		{
			u8"Florian Vaussard",
			u8"florian dot vaussard at bluewin dot ch",
			u8"",
			u8"core software engineer, Windows maintainer until 2015",
			{ u8"core", u8"studio", u8"packaging", }
		},
		{
			u8"David James Sherman",
			u8"david dot sherman at inria dot fr",
			u8"http://www.labri.fr/perso/david/Site/David_James_Sherman.html",
			u8"core software engineer, asebahttp, Zeroconf, Scratch integration",
			{ u8"core", u8"simulator", }
		},
		{
			u8"Jiwon Shin",
			u8"jiwon at shin dot ch",
			u8"",
			u8"software engineer for VPL",
			{ u8"vpl", }
		},
		{
			u8"Thibault Laine",
			u8"thibault dot laine at inria dot fr",
			u8"",
			u8"software engineer for Playground",
			{ u8"simulator", }
		},
		{
			u8"Philippe Rétornaz",
			u8"philippe dot retornaz at a3 dot epfl dot ch",
			u8"",
			u8"software engineer for embedded integration",
			{ u8"core", }
		},
		{
			u8"Fabian Hahn",
			u8"fabian at hahn dot graphics",
			u8"",
			u8"software engineer for Blockly integration",
			{ u8"blockly", }
		},
	};

	static const AuthorInfos thankToList = {
		{
			u8"Maria María Beltrán",
			u8"mari_tran at yahoo dot com",
			u8"",
			u8"VPL re-design - funded by Gebert Rüf Stiftung",
			{ u8"vpl", }
		},
		{
			u8"Fanny Riedo",
			u8"fanny dot riedo at mobsya dot org",
			u8"",
			u8"macOS packaging, translation, quality insurance",
			{ u8"packaging", u8"translation", }
		},
		{
			u8"Michael Bonani",
			u8"michael dot bonani at mobsya dot org",
			u8"",
			u8"French translation, packaging, quality insurance",
			{ u8"packaging", u8"translation", }
		},
		{
			u8"Dean Brettle",
			u8"dean at brettle dot com",
			u8"",
			u8"RPM packaging",
			{ u8"packaging", }
		},
		{
			u8"Martin Voelkle",
			u8"martin dot voelkle at sampla dot ch",
			u8"",
			u8"bug reporting and fixing",
			{ u8"vpl", }
		},
		{
			u8"Yves Piguet",
			u8"yves dot piguet at epfl dot ch",
			u8"",
			u8"bug reporting and fixing",
			{ u8"core", u8"studio", }
		},
		{
			u8"Corentin Jabot",
			u8"corentin dot jabot at gmail dot com",
			u8"",
			u8"bug fixing and coding contributions",
			{ u8"core", }
		},
		{
			u8"Valentin Longchamp",
			u8"valentin dot longchamp at gmail dot com",
			u8"",
			u8"inputs on framework architecture, additional coding",
			{ u8"core", }
		},
		{
			u8"Severin Klingler",
			u8"severin dot klinger at ethz dot ch",
			u8"",
			u8"VPL logging",
			{ u8"vpl", }
		},
		{
			u8"Vincent Becker",
			u8"vincent dot becker at mobsya dot org",
			u8"",
			u8"Textures for Thymio 3D model",
			{ u8"simulator", }
		},
		{
			u8"Francesco Mondada",
			u8"francesco dot mondada at epfl dot ch",
			u8"https://people.epfl.ch/francesco.mondada",
			u8"bug reporting and dissemination",
			{ u8"studio", u8"vpl", u8"blockly", }
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
			u8"basilio dot noris at gmail dot com",
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
			u8"fboteroh at gmail dot com",
			u8"",
			u8"Spanish translation",
			{ u8"translation", }
		},
		{
			u8"Ezio Somá",
			u8"ezio dot soma at gmail dot com",
			u8"",
			u8"Italian translation",
			{ u8"translation", }
		},
		{
			u8"Detlef Rick",
			u8"detlef dot rick at gymnasium-hittfeld dot de",
			u8"",
			u8"German translation",
			{ u8"translation", }
		},
		{
			u8"Shiling Wang",
			u8"shilingwang0621 at gmail dot com",
			u8"",
			u8"Chinese translation",
			{ u8"translation", }
		},
		{
			u8"Mamoru Oichi",
			u8"moichi at exseed-inc dot jp",
			u8"",
			u8"Japanese translation",
			{ u8"translation", }
		},
		{
			u8"Ioanna Theodoropoulou",
			u8"ioannatheodoropou at hotmail dot com",
			u8"",
			u8"Greek translation",
			{ u8"translation", }
		},
		{
			u8"Vassilis Komis",
			u8"komis at upatras dot gr",
			u8"",
			u8"Greek translation",
			{ u8"translation", }
		},
		{
			u8"Basil Stotz",
			u8"basil dot stotz at gmail dot com",
			u8"",
			u8"Italian translation",
			{ u8"translation", }
		},
		{
			u8"Paolo Rossetti",
			u8"paolo dot rossetti at mobsya dot org",
			u8"",
			u8"Italian translation",
			{ u8"translation", }
		},
	};
} // namespace Aseba

#endif // __ASEBA_AUTHORS_H
