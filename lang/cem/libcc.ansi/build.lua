include("plat/build.lua")

tabgen {
	name = "ctype_tab",
	srcs = { "./pure/ctype/char.tab" }
}

normalrule {
	name = "ctype_files",
	ins = { "./pure/ctype/genfiles" },
	outleaves = {
		"isalnum.c",
		"isalpha.c",
		"isascii.c",
		"iscntrl.c",
		"isdigit.c",
		"isgraph.c",
		"islower.c",
		"isprint.c",
		"ispunct.c",
		"isspace.c",
		"isupper.c",
		"isxdigit.c",
	},
	commands = {
		"sh %{ins[1]} %{dir}"
	}
}

for _, plat in ipairs(vars.plats) do
	acklibrary {
		name = "lib_"..plat,
		srcs = {
			"+ctype_files",
			"+ctype_tab",
			"./pure/string/*.c",
			"./pure/stdlib/*.c",
			"./pure/locale/*.c",
			"./pure/setjmp/*.c",
			"./pure/setjmp/*.e",
			"./pure/math/*.c", -- hypot.c
			"./pure/math/*.e",
			"./pure/ctype/*.c",
			"./errno/*.c",
			"./malloc/*.c",
			"./misc/environ.c", -- don't build everything here as it's all obsolete
			"./signal/*.c",
			"./assert/*.c",
			"./stdio/*.c",
			"./stdlib/*.c",
			"./string/*.c",
			"./time/*.c",
		},
		hdrs = {}, -- must be empty
		deps = {
			"lang/cem/libcc.ansi/headers+pkg",
			"plat/"..plat.."/include+pkg",
			"./malloc/malloc.h",
			"./pure/math/localmath.h",
			"./stdio/loc_incl.h",
			"./stdlib/ext_fmt.h",
			"./time/loc_time.h",
		},
		vars = { plat = plat }
	}

	ackfile {
		name = "crt_"..plat,
		srcs = { "./head_ac.e" },
		vars = { plat = plat },
		deps = {
			"h+emheaders"
		}
	}

	local suffix = plat:find("^em") and "m" or "o"
	installable {
		name = "pkg_"..plat,
		map = {
			"lang/cem/libcc.ansi/headers+pkg",
			["$(PLATIND)/"..plat.."/c-ansi."..suffix] = "+crt_"..plat,
			["$(PLATIND)/"..plat.."/libc.a"] = "+lib_"..plat,
		}
	}
end

