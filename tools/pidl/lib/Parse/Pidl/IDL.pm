####################################################################
#
#    This file was generated using Parse::Yapp version 1.05.
#
#        Don't edit this file, use source file instead.
#
#             ANY CHANGE MADE HERE WILL BE LOST !
#
####################################################################
package Parse::Pidl::IDL;
use vars qw ( @ISA );
use strict;

@ISA= qw ( Parse::Yapp::Driver );
use Parse::Yapp::Driver;



sub new {
        my($class)=shift;
        ref($class)
    and $class=ref($class);

    my($self)=$class->SUPER::new( yyversion => '1.05',
                                  yystates =>
[
	{#State 0
		DEFAULT => -1,
		GOTOS => {
			'idl' => 1
		}
	},
	{#State 1
		ACTIONS => {
			'' => 2,
			"cpp_quote" => 3,
			"importlib" => 4,
			"import" => 7,
			"include" => 13
		},
		DEFAULT => -90,
		GOTOS => {
			'cpp_quote' => 11,
			'importlib' => 10,
			'interface' => 9,
			'include' => 5,
			'coclass' => 12,
			'import' => 8,
			'property_list' => 6
		}
	},
	{#State 2
		DEFAULT => 0
	},
	{#State 3
		ACTIONS => {
			"(" => 14
		}
	},
	{#State 4
		ACTIONS => {
			'TEXT' => 16
		},
		GOTOS => {
			'commalist' => 15,
			'text' => 17
		}
	},
	{#State 5
		DEFAULT => -5
	},
	{#State 6
		ACTIONS => {
			"coclass" => 18,
			"[" => 20,
			"interface" => 19
		}
	},
	{#State 7
		ACTIONS => {
			'TEXT' => 16
		},
		GOTOS => {
			'commalist' => 21,
			'text' => 17
		}
	},
	{#State 8
		DEFAULT => -4
	},
	{#State 9
		DEFAULT => -2
	},
	{#State 10
		DEFAULT => -6
	},
	{#State 11
		DEFAULT => -7
	},
	{#State 12
		DEFAULT => -3
	},
	{#State 13
		ACTIONS => {
			'TEXT' => 16
		},
		GOTOS => {
			'commalist' => 22,
			'text' => 17
		}
	},
	{#State 14
		ACTIONS => {
			'TEXT' => 16
		},
		GOTOS => {
			'text' => 23
		}
	},
	{#State 15
		ACTIONS => {
			";" => 24,
			"," => 25
		}
	},
	{#State 16
		DEFAULT => -121
	},
	{#State 17
		DEFAULT => -11
	},
	{#State 18
		ACTIONS => {
			'IDENTIFIER' => 26
		},
		GOTOS => {
			'identifier' => 27
		}
	},
	{#State 19
		ACTIONS => {
			'IDENTIFIER' => 26
		},
		GOTOS => {
			'identifier' => 28
		}
	},
	{#State 20
		ACTIONS => {
			'IDENTIFIER' => 26
		},
		GOTOS => {
			'identifier' => 30,
			'property' => 31,
			'properties' => 29
		}
	},
	{#State 21
		ACTIONS => {
			";" => 32,
			"," => 25
		}
	},
	{#State 22
		ACTIONS => {
			";" => 33,
			"," => 25
		}
	},
	{#State 23
		ACTIONS => {
			")" => 34
		}
	},
	{#State 24
		DEFAULT => -10
	},
	{#State 25
		ACTIONS => {
			'TEXT' => 16
		},
		GOTOS => {
			'text' => 35
		}
	},
	{#State 26
		DEFAULT => -117
	},
	{#State 27
		ACTIONS => {
			"{" => 36
		}
	},
	{#State 28
		ACTIONS => {
			":" => 37
		},
		DEFAULT => -17,
		GOTOS => {
			'base_interface' => 38
		}
	},
	{#State 29
		ACTIONS => {
			"," => 39,
			"]" => 40
		}
	},
	{#State 30
		ACTIONS => {
			"(" => 41
		},
		DEFAULT => -94
	},
	{#State 31
		DEFAULT => -92
	},
	{#State 32
		DEFAULT => -8
	},
	{#State 33
		DEFAULT => -9
	},
	{#State 34
		DEFAULT => -19
	},
	{#State 35
		DEFAULT => -12
	},
	{#State 36
		DEFAULT => -14,
		GOTOS => {
			'interface_names' => 42
		}
	},
	{#State 37
		ACTIONS => {
			'IDENTIFIER' => 26
		},
		GOTOS => {
			'identifier' => 43
		}
	},
	{#State 38
		ACTIONS => {
			"{" => 44
		}
	},
	{#State 39
		ACTIONS => {
			'IDENTIFIER' => 26
		},
		GOTOS => {
			'identifier' => 30,
			'property' => 45
		}
	},
	{#State 40
		DEFAULT => -91
	},
	{#State 41
		ACTIONS => {
			'CONSTANT' => 48,
			'TEXT' => 16,
			'IDENTIFIER' => 26
		},
		DEFAULT => -98,
		GOTOS => {
			'identifier' => 50,
			'text' => 51,
			'anytext' => 46,
			'constant' => 47,
			'commalisttext' => 49
		}
	},
	{#State 42
		ACTIONS => {
			"}" => 52,
			"interface" => 53
		}
	},
	{#State 43
		DEFAULT => -18
	},
	{#State 44
		ACTIONS => {
			"cpp_quote" => 3,
			"const" => 61
		},
		DEFAULT => -90,
		GOTOS => {
			'typedecl' => 64,
			'function' => 54,
			'pipe' => 55,
			'bitmap' => 65,
			'definitions' => 56,
			'definition' => 59,
			'property_list' => 58,
			'usertype' => 57,
			'const' => 66,
			'struct' => 60,
			'cpp_quote' => 67,
			'typedef' => 63,
			'enum' => 62,
			'union' => 68
		}
	},
	{#State 45
		DEFAULT => -93
	},
	{#State 46
		ACTIONS => {
			"-" => 70,
			":" => 69,
			"<" => 71,
			"+" => 73,
			"~" => 72,
			"*" => 74,
			"?" => 75,
			"{" => 76,
			"&" => 77,
			"/" => 78,
			"=" => 79,
			"(" => 80,
			"|" => 81,
			"." => 82,
			">" => 83
		},
		DEFAULT => -96
	},
	{#State 47
		DEFAULT => -100
	},
	{#State 48
		DEFAULT => -120
	},
	{#State 49
		ACTIONS => {
			"," => 84,
			")" => 85
		}
	},
	{#State 50
		DEFAULT => -99
	},
	{#State 51
		DEFAULT => -101
	},
	{#State 52
		ACTIONS => {
			";" => 86
		},
		DEFAULT => -122,
		GOTOS => {
			'optional_semicolon' => 87
		}
	},
	{#State 53
		ACTIONS => {
			'IDENTIFIER' => 26
		},
		GOTOS => {
			'identifier' => 88
		}
	},
	{#State 54
		DEFAULT => -22
	},
	{#State 55
		DEFAULT => -35
	},
	{#State 56
		ACTIONS => {
			"}" => 89,
			"cpp_quote" => 3,
			"const" => 61
		},
		DEFAULT => -90,
		GOTOS => {
			'typedecl' => 64,
			'function' => 54,
			'pipe' => 55,
			'bitmap' => 65,
			'definition' => 90,
			'property_list' => 58,
			'usertype' => 57,
			'const' => 66,
			'struct' => 60,
			'cpp_quote' => 67,
			'enum' => 62,
			'typedef' => 63,
			'union' => 68
		}
	},
	{#State 57
		ACTIONS => {
			";" => 91
		}
	},
	{#State 58
		ACTIONS => {
			"typedef" => 92,
			'IDENTIFIER' => 26,
			"signed" => 101,
			"union" => 93,
			"enum" => 102,
			"bitmap" => 103,
			'void' => 94,
			"pipe" => 104,
			"unsigned" => 105,
			"[" => 20,
			"struct" => 99
		},
		GOTOS => {
			'existingtype' => 100,
			'pipe' => 55,
			'bitmap' => 65,
			'usertype' => 96,
			'property_list' => 95,
			'identifier' => 97,
			'struct' => 60,
			'enum' => 62,
			'type' => 106,
			'union' => 68,
			'sign' => 98
		}
	},
	{#State 59
		DEFAULT => -20
	},
	{#State 60
		DEFAULT => -31
	},
	{#State 61
		ACTIONS => {
			'IDENTIFIER' => 26
		},
		GOTOS => {
			'identifier' => 107
		}
	},
	{#State 62
		DEFAULT => -33
	},
	{#State 63
		DEFAULT => -24
	},
	{#State 64
		DEFAULT => -25
	},
	{#State 65
		DEFAULT => -34
	},
	{#State 66
		DEFAULT => -23
	},
	{#State 67
		DEFAULT => -26
	},
	{#State 68
		DEFAULT => -32
	},
	{#State 69
		ACTIONS => {
			'CONSTANT' => 48,
			'TEXT' => 16,
			'IDENTIFIER' => 26
		},
		DEFAULT => -98,
		GOTOS => {
			'identifier' => 50,
			'anytext' => 108,
			'text' => 51,
			'constant' => 47
		}
	},
	{#State 70
		ACTIONS => {
			'CONSTANT' => 48,
			'TEXT' => 16,
			'IDENTIFIER' => 26
		},
		DEFAULT => -98,
		GOTOS => {
			'identifier' => 50,
			'anytext' => 109,
			'text' => 51,
			'constant' => 47
		}
	},
	{#State 71
		ACTIONS => {
			'CONSTANT' => 48,
			'TEXT' => 16,
			'IDENTIFIER' => 26
		},
		DEFAULT => -98,
		GOTOS => {
			'identifier' => 50,
			'anytext' => 110,
			'text' => 51,
			'constant' => 47
		}
	},
	{#State 72
		ACTIONS => {
			'CONSTANT' => 48,
			'TEXT' => 16,
			'IDENTIFIER' => 26
		},
		DEFAULT => -98,
		GOTOS => {
			'identifier' => 50,
			'anytext' => 111,
			'text' => 51,
			'constant' => 47
		}
	},
	{#State 73
		ACTIONS => {
			'CONSTANT' => 48,
			'TEXT' => 16,
			'IDENTIFIER' => 26
		},
		DEFAULT => -98,
		GOTOS => {
			'identifier' => 50,
			'anytext' => 112,
			'text' => 51,
			'constant' => 47
		}
	},
	{#State 74
		ACTIONS => {
			'CONSTANT' => 48,
			'TEXT' => 16,
			'IDENTIFIER' => 26
		},
		DEFAULT => -98,
		GOTOS => {
			'identifier' => 50,
			'anytext' => 113,
			'text' => 51,
			'constant' => 47
		}
	},
	{#State 75
		ACTIONS => {
			'CONSTANT' => 48,
			'TEXT' => 16,
			'IDENTIFIER' => 26
		},
		DEFAULT => -98,
		GOTOS => {
			'identifier' => 50,
			'anytext' => 114,
			'text' => 51,
			'constant' => 47
		}
	},
	{#State 76
		ACTIONS => {
			'CONSTANT' => 48,
			'TEXT' => 16,
			'IDENTIFIER' => 26
		},
		DEFAULT => -98,
		GOTOS => {
			'identifier' => 50,
			'anytext' => 46,
			'text' => 51,
			'constant' => 47,
			'commalisttext' => 115
		}
	},
	{#State 77
		ACTIONS => {
			'CONSTANT' => 48,
			'TEXT' => 16,
			'IDENTIFIER' => 26
		},
		DEFAULT => -98,
		GOTOS => {
			'identifier' => 50,
			'anytext' => 116,
			'text' => 51,
			'constant' => 47
		}
	},
	{#State 78
		ACTIONS => {
			'CONSTANT' => 48,
			'TEXT' => 16,
			'IDENTIFIER' => 26
		},
		DEFAULT => -98,
		GOTOS => {
			'identifier' => 50,
			'anytext' => 117,
			'text' => 51,
			'constant' => 47
		}
	},
	{#State 79
		ACTIONS => {
			'CONSTANT' => 48,
			'TEXT' => 16,
			'IDENTIFIER' => 26
		},
		DEFAULT => -98,
		GOTOS => {
			'identifier' => 50,
			'anytext' => 118,
			'text' => 51,
			'constant' => 47
		}
	},
	{#State 80
		ACTIONS => {
			'CONSTANT' => 48,
			'TEXT' => 16,
			'IDENTIFIER' => 26
		},
		DEFAULT => -98,
		GOTOS => {
			'identifier' => 50,
			'anytext' => 46,
			'text' => 51,
			'constant' => 47,
			'commalisttext' => 119
		}
	},
	{#State 81
		ACTIONS => {
			'CONSTANT' => 48,
			'TEXT' => 16,
			'IDENTIFIER' => 26
		},
		DEFAULT => -98,
		GOTOS => {
			'identifier' => 50,
			'anytext' => 120,
			'text' => 51,
			'constant' => 47
		}
	},
	{#State 82
		ACTIONS => {
			'CONSTANT' => 48,
			'TEXT' => 16,
			'IDENTIFIER' => 26
		},
		DEFAULT => -98,
		GOTOS => {
			'identifier' => 50,
			'anytext' => 121,
			'text' => 51,
			'constant' => 47
		}
	},
	{#State 83
		ACTIONS => {
			'CONSTANT' => 48,
			'TEXT' => 16,
			'IDENTIFIER' => 26
		},
		DEFAULT => -98,
		GOTOS => {
			'identifier' => 50,
			'anytext' => 122,
			'text' => 51,
			'constant' => 47
		}
	},
	{#State 84
		ACTIONS => {
			'CONSTANT' => 48,
			'TEXT' => 16,
			'IDENTIFIER' => 26
		},
		DEFAULT => -98,
		GOTOS => {
			'identifier' => 50,
			'anytext' => 123,
			'text' => 51,
			'constant' => 47
		}
	},
	{#State 85
		DEFAULT => -95
	},
	{#State 86
		DEFAULT => -123
	},
	{#State 87
		DEFAULT => -13
	},
	{#State 88
		ACTIONS => {
			";" => 124
		}
	},
	{#State 89
		ACTIONS => {
			";" => 86
		},
		DEFAULT => -122,
		GOTOS => {
			'optional_semicolon' => 125
		}
	},
	{#State 90
		DEFAULT => -21
	},
	{#State 91
		DEFAULT => -36
	},
	{#State 92
		ACTIONS => {
			"const" => 127
		},
		DEFAULT => -81,
		GOTOS => {
			'optional_const' => 126
		}
	},
	{#State 93
		ACTIONS => {
			'IDENTIFIER' => 128
		},
		DEFAULT => -118,
		GOTOS => {
			'optional_identifier' => 129
		}
	},
	{#State 94
		DEFAULT => -43
	},
	{#State 95
		ACTIONS => {
			"pipe" => 104,
			"union" => 93,
			"enum" => 102,
			"bitmap" => 103,
			"[" => 20,
			"struct" => 99
		}
	},
	{#State 96
		DEFAULT => -41
	},
	{#State 97
		DEFAULT => -40
	},
	{#State 98
		ACTIONS => {
			'IDENTIFIER' => 26
		},
		GOTOS => {
			'identifier' => 130
		}
	},
	{#State 99
		ACTIONS => {
			'IDENTIFIER' => 128
		},
		DEFAULT => -118,
		GOTOS => {
			'optional_identifier' => 131
		}
	},
	{#State 100
		DEFAULT => -42
	},
	{#State 101
		DEFAULT => -37
	},
	{#State 102
		ACTIONS => {
			'IDENTIFIER' => 128
		},
		DEFAULT => -118,
		GOTOS => {
			'optional_identifier' => 132
		}
	},
	{#State 103
		ACTIONS => {
			'IDENTIFIER' => 128
		},
		DEFAULT => -118,
		GOTOS => {
			'optional_identifier' => 133
		}
	},
	{#State 104
		ACTIONS => {
			'IDENTIFIER' => 26,
			"signed" => 101,
			'void' => 94,
			"unsigned" => 105
		},
		DEFAULT => -90,
		GOTOS => {
			'existingtype' => 100,
			'pipe' => 55,
			'bitmap' => 65,
			'usertype' => 96,
			'property_list' => 95,
			'identifier' => 97,
			'struct' => 60,
			'enum' => 62,
			'type' => 134,
			'union' => 68,
			'sign' => 98
		}
	},
	{#State 105
		DEFAULT => -38
	},
	{#State 106
		ACTIONS => {
			'IDENTIFIER' => 26
		},
		GOTOS => {
			'identifier' => 135
		}
	},
	{#State 107
		DEFAULT => -76,
		GOTOS => {
			'pointers' => 136
		}
	},
	{#State 108
		ACTIONS => {
			"-" => 70,
			":" => 69,
			"<" => 71,
			"+" => 73,
			"~" => 72,
			"*" => 74,
			"?" => 75,
			"{" => 76,
			"&" => 77,
			"/" => 78,
			"=" => 79,
			"(" => 80,
			"|" => 81,
			"." => 82,
			">" => 83
		},
		DEFAULT => -111
	},
	{#State 109
		ACTIONS => {
			":" => 69,
			"<" => 71,
			"~" => 72,
			"?" => 75,
			"{" => 76,
			"=" => 79
		},
		DEFAULT => -102
	},
	{#State 110
		ACTIONS => {
			"-" => 70,
			":" => 69,
			"<" => 71,
			"+" => 73,
			"~" => 72,
			"*" => 74,
			"?" => 75,
			"{" => 76,
			"&" => 77,
			"/" => 78,
			"=" => 79,
			"(" => 80,
			"|" => 81,
			"." => 82,
			">" => 83
		},
		DEFAULT => -106
	},
	{#State 111
		ACTIONS => {
			"-" => 70,
			":" => 69,
			"<" => 71,
			"+" => 73,
			"~" => 72,
			"*" => 74,
			"?" => 75,
			"{" => 76,
			"&" => 77,
			"/" => 78,
			"=" => 79,
			"(" => 80,
			"|" => 81,
			"." => 82,
			">" => 83
		},
		DEFAULT => -114
	},
	{#State 112
		ACTIONS => {
			":" => 69,
			"<" => 71,
			"~" => 72,
			"?" => 75,
			"{" => 76,
			"=" => 79
		},
		DEFAULT => -113
	},
	{#State 113
		ACTIONS => {
			":" => 69,
			"<" => 71,
			"~" => 72,
			"?" => 75,
			"{" => 76,
			"=" => 79
		},
		DEFAULT => -104
	},
	{#State 114
		ACTIONS => {
			"-" => 70,
			":" => 69,
			"<" => 71,
			"+" => 73,
			"~" => 72,
			"*" => 74,
			"?" => 75,
			"{" => 76,
			"&" => 77,
			"/" => 78,
			"=" => 79,
			"(" => 80,
			"|" => 81,
			"." => 82,
			">" => 83
		},
		DEFAULT => -110
	},
	{#State 115
		ACTIONS => {
			"}" => 137,
			"," => 84
		}
	},
	{#State 116
		ACTIONS => {
			":" => 69,
			"<" => 71,
			"~" => 72,
			"?" => 75,
			"{" => 76,
			"=" => 79
		},
		DEFAULT => -108
	},
	{#State 117
		ACTIONS => {
			":" => 69,
			"<" => 71,
			"~" => 72,
			"?" => 75,
			"{" => 76,
			"=" => 79
		},
		DEFAULT => -109
	},
	{#State 118
		ACTIONS => {
			"-" => 70,
			":" => 69,
			"<" => 71,
			"+" => 73,
			"~" => 72,
			"*" => 74,
			"?" => 75,
			"{" => 76,
			"&" => 77,
			"/" => 78,
			"=" => 79,
			"(" => 80,
			"|" => 81,
			"." => 82,
			">" => 83
		},
		DEFAULT => -112
	},
	{#State 119
		ACTIONS => {
			"," => 84,
			")" => 138
		}
	},
	{#State 120
		ACTIONS => {
			":" => 69,
			"<" => 71,
			"~" => 72,
			"?" => 75,
			"{" => 76,
			"=" => 79
		},
		DEFAULT => -107
	},
	{#State 121
		ACTIONS => {
			":" => 69,
			"<" => 71,
			"~" => 72,
			"?" => 75,
			"{" => 76,
			"=" => 79
		},
		DEFAULT => -103
	},
	{#State 122
		ACTIONS => {
			":" => 69,
			"<" => 71,
			"~" => 72,
			"?" => 75,
			"{" => 76,
			"=" => 79
		},
		DEFAULT => -105
	},
	{#State 123
		ACTIONS => {
			"-" => 70,
			":" => 69,
			"<" => 71,
			"+" => 73,
			"~" => 72,
			"*" => 74,
			"?" => 75,
			"{" => 76,
			"&" => 77,
			"/" => 78,
			"=" => 79,
			"(" => 80,
			"|" => 81,
			"." => 82,
			">" => 83
		},
		DEFAULT => -97
	},
	{#State 124
		DEFAULT => -15
	},
	{#State 125
		DEFAULT => -16
	},
	{#State 126
		ACTIONS => {
			'IDENTIFIER' => 26,
			"signed" => 101,
			'void' => 94,
			"unsigned" => 105
		},
		DEFAULT => -90,
		GOTOS => {
			'existingtype' => 100,
			'pipe' => 55,
			'bitmap' => 65,
			'usertype' => 96,
			'property_list' => 95,
			'identifier' => 97,
			'struct' => 60,
			'enum' => 62,
			'type' => 139,
			'union' => 68,
			'sign' => 98
		}
	},
	{#State 127
		DEFAULT => -82
	},
	{#State 128
		DEFAULT => -119
	},
	{#State 129
		ACTIONS => {
			"{" => 141
		},
		DEFAULT => -72,
		GOTOS => {
			'union_body' => 142,
			'opt_union_body' => 140
		}
	},
	{#State 130
		DEFAULT => -39
	},
	{#State 131
		ACTIONS => {
			"{" => 144
		},
		DEFAULT => -62,
		GOTOS => {
			'struct_body' => 143,
			'opt_struct_body' => 145
		}
	},
	{#State 132
		ACTIONS => {
			"{" => 146
		},
		DEFAULT => -45,
		GOTOS => {
			'opt_enum_body' => 148,
			'enum_body' => 147
		}
	},
	{#State 133
		ACTIONS => {
			"{" => 150
		},
		DEFAULT => -53,
		GOTOS => {
			'bitmap_body' => 151,
			'opt_bitmap_body' => 149
		}
	},
	{#State 134
		DEFAULT => -78
	},
	{#State 135
		ACTIONS => {
			"(" => 152
		}
	},
	{#State 136
		ACTIONS => {
			'IDENTIFIER' => 26,
			"*" => 154
		},
		GOTOS => {
			'identifier' => 153
		}
	},
	{#State 137
		ACTIONS => {
			'CONSTANT' => 48,
			'TEXT' => 16,
			'IDENTIFIER' => 26
		},
		DEFAULT => -98,
		GOTOS => {
			'identifier' => 50,
			'anytext' => 155,
			'text' => 51,
			'constant' => 47
		}
	},
	{#State 138
		ACTIONS => {
			'CONSTANT' => 48,
			'TEXT' => 16,
			'IDENTIFIER' => 26
		},
		DEFAULT => -98,
		GOTOS => {
			'identifier' => 50,
			'anytext' => 156,
			'text' => 51,
			'constant' => 47
		}
	},
	{#State 139
		DEFAULT => -76,
		GOTOS => {
			'pointers' => 157
		}
	},
	{#State 140
		DEFAULT => -74
	},
	{#State 141
		DEFAULT => -69,
		GOTOS => {
			'union_elements' => 158
		}
	},
	{#State 142
		DEFAULT => -73
	},
	{#State 143
		DEFAULT => -63
	},
	{#State 144
		DEFAULT => -79,
		GOTOS => {
			'element_list1' => 159
		}
	},
	{#State 145
		DEFAULT => -64
	},
	{#State 146
		ACTIONS => {
			'IDENTIFIER' => 26
		},
		GOTOS => {
			'identifier' => 160,
			'enum_element' => 161,
			'enum_elements' => 162
		}
	},
	{#State 147
		DEFAULT => -46
	},
	{#State 148
		DEFAULT => -47
	},
	{#State 149
		DEFAULT => -55
	},
	{#State 150
		ACTIONS => {
			'IDENTIFIER' => 26
		},
		DEFAULT => -58,
		GOTOS => {
			'identifier' => 165,
			'bitmap_element' => 164,
			'bitmap_elements' => 163,
			'opt_bitmap_elements' => 166
		}
	},
	{#State 151
		DEFAULT => -54
	},
	{#State 152
		ACTIONS => {
			"," => -83,
			"void" => 169,
			"const" => 127,
			")" => -83
		},
		DEFAULT => -81,
		GOTOS => {
			'optional_const' => 167,
			'element_list2' => 168
		}
	},
	{#State 153
		ACTIONS => {
			"[" => 170,
			"=" => 172
		},
		GOTOS => {
			'array_len' => 171
		}
	},
	{#State 154
		DEFAULT => -77
	},
	{#State 155
		ACTIONS => {
			"-" => 70,
			":" => 69,
			"<" => 71,
			"+" => 73,
			"~" => 72,
			"*" => 74,
			"?" => 75,
			"{" => 76,
			"&" => 77,
			"/" => 78,
			"=" => 79,
			"(" => 80,
			"|" => 81,
			"." => 82,
			">" => 83
		},
		DEFAULT => -116
	},
	{#State 156
		ACTIONS => {
			":" => 69,
			"<" => 71,
			"~" => 72,
			"?" => 75,
			"{" => 76,
			"=" => 79
		},
		DEFAULT => -115
	},
	{#State 157
		ACTIONS => {
			'IDENTIFIER' => 26,
			"*" => 154
		},
		GOTOS => {
			'identifier' => 173
		}
	},
	{#State 158
		ACTIONS => {
			"}" => 174
		},
		DEFAULT => -90,
		GOTOS => {
			'optional_base_element' => 176,
			'property_list' => 175
		}
	},
	{#State 159
		ACTIONS => {
			"}" => 177
		},
		DEFAULT => -90,
		GOTOS => {
			'base_element' => 178,
			'property_list' => 179
		}
	},
	{#State 160
		ACTIONS => {
			"=" => 180
		},
		DEFAULT => -50
	},
	{#State 161
		DEFAULT => -48
	},
	{#State 162
		ACTIONS => {
			"}" => 181,
			"," => 182
		}
	},
	{#State 163
		ACTIONS => {
			"," => 183
		},
		DEFAULT => -59
	},
	{#State 164
		DEFAULT => -56
	},
	{#State 165
		ACTIONS => {
			"=" => 184
		}
	},
	{#State 166
		ACTIONS => {
			"}" => 185
		}
	},
	{#State 167
		DEFAULT => -90,
		GOTOS => {
			'base_element' => 186,
			'property_list' => 179
		}
	},
	{#State 168
		ACTIONS => {
			"," => 187,
			")" => 188
		}
	},
	{#State 169
		DEFAULT => -84
	},
	{#State 170
		ACTIONS => {
			'CONSTANT' => 48,
			'TEXT' => 16,
			"]" => 189,
			'IDENTIFIER' => 26
		},
		DEFAULT => -98,
		GOTOS => {
			'identifier' => 50,
			'anytext' => 190,
			'text' => 51,
			'constant' => 47
		}
	},
	{#State 171
		ACTIONS => {
			"=" => 191
		}
	},
	{#State 172
		ACTIONS => {
			'CONSTANT' => 48,
			'TEXT' => 16,
			'IDENTIFIER' => 26
		},
		DEFAULT => -98,
		GOTOS => {
			'identifier' => 50,
			'anytext' => 192,
			'text' => 51,
			'constant' => 47
		}
	},
	{#State 173
		ACTIONS => {
			"[" => 170
		},
		DEFAULT => -87,
		GOTOS => {
			'array_len' => 193
		}
	},
	{#State 174
		DEFAULT => -71
	},
	{#State 175
		ACTIONS => {
			"[" => 20
		},
		DEFAULT => -90,
		GOTOS => {
			'base_or_empty' => 194,
			'base_element' => 195,
			'empty_element' => 196,
			'property_list' => 197
		}
	},
	{#State 176
		DEFAULT => -70
	},
	{#State 177
		DEFAULT => -61
	},
	{#State 178
		ACTIONS => {
			";" => 198
		}
	},
	{#State 179
		ACTIONS => {
			'IDENTIFIER' => 26,
			"signed" => 101,
			'void' => 94,
			"unsigned" => 105,
			"[" => 20
		},
		DEFAULT => -90,
		GOTOS => {
			'existingtype' => 100,
			'pipe' => 55,
			'bitmap' => 65,
			'usertype' => 96,
			'property_list' => 95,
			'identifier' => 97,
			'struct' => 60,
			'enum' => 62,
			'type' => 199,
			'union' => 68,
			'sign' => 98
		}
	},
	{#State 180
		ACTIONS => {
			'CONSTANT' => 48,
			'TEXT' => 16,
			'IDENTIFIER' => 26
		},
		DEFAULT => -98,
		GOTOS => {
			'identifier' => 50,
			'anytext' => 200,
			'text' => 51,
			'constant' => 47
		}
	},
	{#State 181
		DEFAULT => -44
	},
	{#State 182
		ACTIONS => {
			'IDENTIFIER' => 26
		},
		GOTOS => {
			'identifier' => 160,
			'enum_element' => 201
		}
	},
	{#State 183
		ACTIONS => {
			'IDENTIFIER' => 26
		},
		GOTOS => {
			'identifier' => 165,
			'bitmap_element' => 202
		}
	},
	{#State 184
		ACTIONS => {
			'CONSTANT' => 48,
			'TEXT' => 16,
			'IDENTIFIER' => 26
		},
		DEFAULT => -98,
		GOTOS => {
			'identifier' => 50,
			'anytext' => 203,
			'text' => 51,
			'constant' => 47
		}
	},
	{#State 185
		DEFAULT => -52
	},
	{#State 186
		DEFAULT => -85
	},
	{#State 187
		ACTIONS => {
			"const" => 127
		},
		DEFAULT => -81,
		GOTOS => {
			'optional_const' => 204
		}
	},
	{#State 188
		ACTIONS => {
			";" => 205
		}
	},
	{#State 189
		ACTIONS => {
			"[" => 170
		},
		DEFAULT => -87,
		GOTOS => {
			'array_len' => 206
		}
	},
	{#State 190
		ACTIONS => {
			"-" => 70,
			":" => 69,
			"<" => 71,
			"+" => 73,
			"~" => 72,
			"*" => 74,
			"]" => 207,
			"?" => 75,
			"{" => 76,
			"&" => 77,
			"/" => 78,
			"=" => 79,
			"(" => 80,
			"|" => 81,
			"." => 82,
			">" => 83
		}
	},
	{#State 191
		ACTIONS => {
			'CONSTANT' => 48,
			'TEXT' => 16,
			'IDENTIFIER' => 26
		},
		DEFAULT => -98,
		GOTOS => {
			'identifier' => 50,
			'anytext' => 208,
			'text' => 51,
			'constant' => 47
		}
	},
	{#State 192
		ACTIONS => {
			"-" => 70,
			":" => 69,
			"<" => 71,
			";" => 209,
			"+" => 73,
			"~" => 72,
			"*" => 74,
			"?" => 75,
			"{" => 76,
			"&" => 77,
			"/" => 78,
			"=" => 79,
			"(" => 80,
			"|" => 81,
			"." => 82,
			">" => 83
		}
	},
	{#State 193
		ACTIONS => {
			";" => 210
		}
	},
	{#State 194
		DEFAULT => -68
	},
	{#State 195
		ACTIONS => {
			";" => 211
		}
	},
	{#State 196
		DEFAULT => -67
	},
	{#State 197
		ACTIONS => {
			'IDENTIFIER' => 26,
			"signed" => 101,
			";" => 212,
			'void' => 94,
			"unsigned" => 105,
			"[" => 20
		},
		DEFAULT => -90,
		GOTOS => {
			'existingtype' => 100,
			'pipe' => 55,
			'bitmap' => 65,
			'usertype' => 96,
			'property_list' => 95,
			'identifier' => 97,
			'struct' => 60,
			'enum' => 62,
			'type' => 199,
			'union' => 68,
			'sign' => 98
		}
	},
	{#State 198
		DEFAULT => -80
	},
	{#State 199
		DEFAULT => -76,
		GOTOS => {
			'pointers' => 213
		}
	},
	{#State 200
		ACTIONS => {
			"-" => 70,
			":" => 69,
			"<" => 71,
			"+" => 73,
			"~" => 72,
			"*" => 74,
			"?" => 75,
			"{" => 76,
			"&" => 77,
			"/" => 78,
			"=" => 79,
			"(" => 80,
			"|" => 81,
			"." => 82,
			">" => 83
		},
		DEFAULT => -51
	},
	{#State 201
		DEFAULT => -49
	},
	{#State 202
		DEFAULT => -57
	},
	{#State 203
		ACTIONS => {
			"-" => 70,
			":" => 69,
			"<" => 71,
			"+" => 73,
			"~" => 72,
			"*" => 74,
			"?" => 75,
			"{" => 76,
			"&" => 77,
			"/" => 78,
			"=" => 79,
			"(" => 80,
			"|" => 81,
			"." => 82,
			">" => 83
		},
		DEFAULT => -60
	},
	{#State 204
		DEFAULT => -90,
		GOTOS => {
			'base_element' => 214,
			'property_list' => 179
		}
	},
	{#State 205
		DEFAULT => -29
	},
	{#State 206
		DEFAULT => -88
	},
	{#State 207
		ACTIONS => {
			"[" => 170
		},
		DEFAULT => -87,
		GOTOS => {
			'array_len' => 215
		}
	},
	{#State 208
		ACTIONS => {
			"-" => 70,
			":" => 69,
			"<" => 71,
			";" => 216,
			"+" => 73,
			"~" => 72,
			"*" => 74,
			"?" => 75,
			"{" => 76,
			"&" => 77,
			"/" => 78,
			"=" => 79,
			"(" => 80,
			"|" => 81,
			"." => 82,
			">" => 83
		}
	},
	{#State 209
		DEFAULT => -27
	},
	{#State 210
		DEFAULT => -30
	},
	{#State 211
		DEFAULT => -66
	},
	{#State 212
		DEFAULT => -65
	},
	{#State 213
		ACTIONS => {
			'IDENTIFIER' => 26,
			"*" => 154
		},
		GOTOS => {
			'identifier' => 217
		}
	},
	{#State 214
		DEFAULT => -86
	},
	{#State 215
		DEFAULT => -89
	},
	{#State 216
		DEFAULT => -28
	},
	{#State 217
		ACTIONS => {
			"[" => 170
		},
		DEFAULT => -87,
		GOTOS => {
			'array_len' => 218
		}
	},
	{#State 218
		DEFAULT => -75
	}
],
                                  yyrules  =>
[
	[#Rule 0
		 '$start', 2, undef
	],
	[#Rule 1
		 'idl', 0, undef
	],
	[#Rule 2
		 'idl', 2,
sub
#line 20 "idl.yp"
{ push(@{$_[1]}, $_[2]); $_[1] }
	],
	[#Rule 3
		 'idl', 2,
sub
#line 22 "idl.yp"
{ push(@{$_[1]}, $_[2]); $_[1] }
	],
	[#Rule 4
		 'idl', 2,
sub
#line 24 "idl.yp"
{ push(@{$_[1]}, $_[2]); $_[1] }
	],
	[#Rule 5
		 'idl', 2,
sub
#line 26 "idl.yp"
{ push(@{$_[1]}, $_[2]); $_[1] }
	],
	[#Rule 6
		 'idl', 2,
sub
#line 28 "idl.yp"
{ push(@{$_[1]}, $_[2]); $_[1] }
	],
	[#Rule 7
		 'idl', 2,
sub
#line 30 "idl.yp"
{ push(@{$_[1]}, $_[2]); $_[1] }
	],
	[#Rule 8
		 'import', 3,
sub
#line 35 "idl.yp"
{{
		"TYPE" => "IMPORT",
		"PATHS" => $_[2],
		"FILE" => $_[0]->YYData->{FILE},
		"LINE" => $_[0]->YYData->{LINE},
	}}
	],
	[#Rule 9
		 'include', 3,
sub
#line 45 "idl.yp"
{{
		"TYPE" => "INCLUDE",
		"PATHS" => $_[2],
		"FILE" => $_[0]->YYData->{FILE},
		"LINE" => $_[0]->YYData->{LINE},
	}}
	],
	[#Rule 10
		 'importlib', 3,
sub
#line 55 "idl.yp"
{{
		"TYPE" => "IMPORTLIB",
		"PATHS" => $_[2],
		"FILE" => $_[0]->YYData->{FILE},
		"LINE" => $_[0]->YYData->{LINE},
	}}
	],
	[#Rule 11
		 'commalist', 1,
sub
#line 64 "idl.yp"
{ [ $_[1] ] }
	],
	[#Rule 12
		 'commalist', 3,
sub
#line 66 "idl.yp"
{ push(@{$_[1]}, $_[3]); $_[1] }
	],
	[#Rule 13
		 'coclass', 7,
sub
#line 71 "idl.yp"
{{
		"TYPE" => "COCLASS",
		"PROPERTIES" => $_[1],
		"NAME" => $_[3],
		"DATA" => $_[5],
		"FILE" => $_[0]->YYData->{FILE},
		"LINE" => $_[0]->YYData->{LINE},
	}}
	],
	[#Rule 14
		 'interface_names', 0, undef
	],
	[#Rule 15
		 'interface_names', 4,
sub
#line 84 "idl.yp"
{ push(@{$_[1]}, $_[2]); $_[1] }
	],
	[#Rule 16
		 'interface', 8,
sub
#line 89 "idl.yp"
{{
		"TYPE" => "INTERFACE",
		"PROPERTIES" => $_[1],
		"NAME" => $_[3],
		"BASE" => $_[4],
		"DATA" => $_[6],
		"FILE" => $_[0]->YYData->{FILE},
		"LINE" => $_[0]->YYData->{LINE},
	}}
	],
	[#Rule 17
		 'base_interface', 0, undef
	],
	[#Rule 18
		 'base_interface', 2,
sub
#line 103 "idl.yp"
{ $_[2] }
	],
	[#Rule 19
		 'cpp_quote', 4,
sub
#line 109 "idl.yp"
{{
		 "TYPE" => "CPP_QUOTE",
		 "DATA" => $_[3],
		 "FILE" => $_[0]->YYData->{FILE},
		 "LINE" => $_[0]->YYData->{LINE},
	}}
	],
	[#Rule 20
		 'definitions', 1,
sub
#line 118 "idl.yp"
{ [ $_[1] ] }
	],
	[#Rule 21
		 'definitions', 2,
sub
#line 120 "idl.yp"
{ push(@{$_[1]}, $_[2]); $_[1] }
	],
	[#Rule 22
		 'definition', 1, undef
	],
	[#Rule 23
		 'definition', 1, undef
	],
	[#Rule 24
		 'definition', 1, undef
	],
	[#Rule 25
		 'definition', 1, undef
	],
	[#Rule 26
		 'definition', 1, undef
	],
	[#Rule 27
		 'const', 7,
sub
#line 137 "idl.yp"
{{
		"TYPE"  => "CONST",
		"DTYPE"  => $_[2],
		"POINTERS" => $_[3],
		"NAME"  => $_[4],
		"VALUE" => $_[6],
		"FILE" => $_[0]->YYData->{FILE},
		"LINE" => $_[0]->YYData->{LINE},
	}}
	],
	[#Rule 28
		 'const', 8,
sub
#line 148 "idl.yp"
{{
		"TYPE"  => "CONST",
		"DTYPE"  => $_[2],
		"POINTERS" => $_[3],
		"NAME"  => $_[4],
		"ARRAY_LEN" => $_[5],
		"VALUE" => $_[7],
		"FILE" => $_[0]->YYData->{FILE},
		"LINE" => $_[0]->YYData->{LINE},
	}}
	],
	[#Rule 29
		 'function', 7,
sub
#line 162 "idl.yp"
{{
		"TYPE" => "FUNCTION",
		"NAME" => $_[3],
		"RETURN_TYPE" => $_[2],
		"PROPERTIES" => $_[1],
		"ELEMENTS" => $_[5],
		"FILE" => $_[0]->YYData->{FILE},
		"LINE" => $_[0]->YYData->{LINE},
	}}
	],
	[#Rule 30
		 'typedef', 8,
sub
#line 175 "idl.yp"
{{
		"TYPE" => "TYPEDEF",
		"PROPERTIES" => $_[1],
		"CONST" => $_[3],
		"NAME" => $_[6],
		"DATA" => $_[4],
		"POINTERS" => $_[5],
		"ARRAY_LEN" => $_[7],
		"FILE" => $_[0]->YYData->{FILE},
		"LINE" => $_[0]->YYData->{LINE},
        }}
	],
	[#Rule 31
		 'usertype', 1, undef
	],
	[#Rule 32
		 'usertype', 1, undef
	],
	[#Rule 33
		 'usertype', 1, undef
	],
	[#Rule 34
		 'usertype', 1, undef
	],
	[#Rule 35
		 'usertype', 1, undef
	],
	[#Rule 36
		 'typedecl', 2,
sub
#line 201 "idl.yp"
{ $_[1] }
	],
	[#Rule 37
		 'sign', 1, undef
	],
	[#Rule 38
		 'sign', 1, undef
	],
	[#Rule 39
		 'existingtype', 2,
sub
#line 211 "idl.yp"
{ ($_[1]?$_[1]:"signed") ." $_[2]" }
	],
	[#Rule 40
		 'existingtype', 1, undef
	],
	[#Rule 41
		 'type', 1, undef
	],
	[#Rule 42
		 'type', 1, undef
	],
	[#Rule 43
		 'type', 1,
sub
#line 221 "idl.yp"
{ "void" }
	],
	[#Rule 44
		 'enum_body', 3,
sub
#line 225 "idl.yp"
{ $_[2] }
	],
	[#Rule 45
		 'opt_enum_body', 0, undef
	],
	[#Rule 46
		 'opt_enum_body', 1, undef
	],
	[#Rule 47
		 'enum', 4,
sub
#line 236 "idl.yp"
{{
		"TYPE" => "ENUM",
		"PROPERTIES" => $_[1],
		"NAME" => $_[3],
		"ELEMENTS" => $_[4],
		"FILE" => $_[0]->YYData->{FILE},
		"LINE" => $_[0]->YYData->{LINE},
	}}
	],
	[#Rule 48
		 'enum_elements', 1,
sub
#line 247 "idl.yp"
{ [ $_[1] ] }
	],
	[#Rule 49
		 'enum_elements', 3,
sub
#line 249 "idl.yp"
{ push(@{$_[1]}, $_[3]); $_[1] }
	],
	[#Rule 50
		 'enum_element', 1, undef
	],
	[#Rule 51
		 'enum_element', 3,
sub
#line 255 "idl.yp"
{ "$_[1]$_[2]$_[3]" }
	],
	[#Rule 52
		 'bitmap_body', 3,
sub
#line 259 "idl.yp"
{ $_[2] }
	],
	[#Rule 53
		 'opt_bitmap_body', 0, undef
	],
	[#Rule 54
		 'opt_bitmap_body', 1, undef
	],
	[#Rule 55
		 'bitmap', 4,
sub
#line 270 "idl.yp"
{{
		"TYPE" => "BITMAP",
		"PROPERTIES" => $_[1],
		"NAME" => $_[3],
		"ELEMENTS" => $_[4],
		"FILE" => $_[0]->YYData->{FILE},
		"LINE" => $_[0]->YYData->{LINE},
	}}
	],
	[#Rule 56
		 'bitmap_elements', 1,
sub
#line 281 "idl.yp"
{ [ $_[1] ] }
	],
	[#Rule 57
		 'bitmap_elements', 3,
sub
#line 283 "idl.yp"
{ push(@{$_[1]}, $_[3]); $_[1] }
	],
	[#Rule 58
		 'opt_bitmap_elements', 0, undef
	],
	[#Rule 59
		 'opt_bitmap_elements', 1, undef
	],
	[#Rule 60
		 'bitmap_element', 3,
sub
#line 293 "idl.yp"
{ "$_[1] ( $_[3] )" }
	],
	[#Rule 61
		 'struct_body', 3,
sub
#line 297 "idl.yp"
{ $_[2] }
	],
	[#Rule 62
		 'opt_struct_body', 0, undef
	],
	[#Rule 63
		 'opt_struct_body', 1, undef
	],
	[#Rule 64
		 'struct', 4,
sub
#line 308 "idl.yp"
{{
		"TYPE" => "STRUCT",
		"PROPERTIES" => $_[1],
		"NAME" => $_[3],
		"ELEMENTS" => $_[4],
		"FILE" => $_[0]->YYData->{FILE},
		"LINE" => $_[0]->YYData->{LINE},
	}}
	],
	[#Rule 65
		 'empty_element', 2,
sub
#line 320 "idl.yp"
{{
		"NAME" => "",
		"TYPE" => "EMPTY",
		"PROPERTIES" => $_[1],
		"POINTERS" => 0,
		"ARRAY_LEN" => [],
		"FILE" => $_[0]->YYData->{FILE},
		"LINE" => $_[0]->YYData->{LINE},
	}}
	],
	[#Rule 66
		 'base_or_empty', 2, undef
	],
	[#Rule 67
		 'base_or_empty', 1, undef
	],
	[#Rule 68
		 'optional_base_element', 2,
sub
#line 337 "idl.yp"
{ $_[2]->{PROPERTIES} = FlattenHash([$_[1],$_[2]->{PROPERTIES}]); $_[2] }
	],
	[#Rule 69
		 'union_elements', 0, undef
	],
	[#Rule 70
		 'union_elements', 2,
sub
#line 343 "idl.yp"
{ push(@{$_[1]}, $_[2]); $_[1] }
	],
	[#Rule 71
		 'union_body', 3,
sub
#line 347 "idl.yp"
{ $_[2] }
	],
	[#Rule 72
		 'opt_union_body', 0, undef
	],
	[#Rule 73
		 'opt_union_body', 1, undef
	],
	[#Rule 74
		 'union', 4,
sub
#line 358 "idl.yp"
{{
		"TYPE" => "UNION",
		"PROPERTIES" => $_[1],
		"NAME" => $_[3],
		"ELEMENTS" => $_[4],
		"FILE" => $_[0]->YYData->{FILE},
		"LINE" => $_[0]->YYData->{LINE},
	}}
	],
	[#Rule 75
		 'base_element', 5,
sub
#line 370 "idl.yp"
{{
		"NAME" => $_[4],
		"TYPE" => $_[2],
		"PROPERTIES" => $_[1],
		"POINTERS" => $_[3],
		"ARRAY_LEN" => $_[5],
		"FILE" => $_[0]->YYData->{FILE},
		"LINE" => $_[0]->YYData->{LINE},
	}}
	],
	[#Rule 76
		 'pointers', 0,
sub
#line 383 "idl.yp"
{ 0 }
	],
	[#Rule 77
		 'pointers', 2,
sub
#line 385 "idl.yp"
{ $_[1]+1 }
	],
	[#Rule 78
		 'pipe', 3,
sub
#line 390 "idl.yp"
{{
		"TYPE" => "PIPE",
		"PROPERTIES" => $_[1],
		"NAME" => undef,
		"DATA" => {
			"TYPE" => "STRUCT",
			"PROPERTIES" => $_[1],
			"NAME" => undef,
			"ELEMENTS" => [{
				"NAME" => "count",
				"PROPERTIES" => $_[1],
				"POINTERS" => 0,
				"ARRAY_LEN" => [],
				"TYPE" => "uint3264",
				"FILE" => $_[0]->YYData->{FILE},
				"LINE" => $_[0]->YYData->{LINE},
			},{
				"NAME" => "array",
				"PROPERTIES" => $_[1],
				"POINTERS" => 0,
				"ARRAY_LEN" => [ "count" ],
				"TYPE" => $_[3],
				"FILE" => $_[0]->YYData->{FILE},
				"LINE" => $_[0]->YYData->{LINE},
			}],
			"FILE" => $_[0]->YYData->{FILE},
			"LINE" => $_[0]->YYData->{LINE},
		},
		"FILE" => $_[0]->YYData->{FILE},
		"LINE" => $_[0]->YYData->{LINE},
	}}
	],
	[#Rule 79
		 'element_list1', 0,
sub
#line 425 "idl.yp"
{ [] }
	],
	[#Rule 80
		 'element_list1', 3,
sub
#line 427 "idl.yp"
{ push(@{$_[1]}, $_[2]); $_[1] }
	],
	[#Rule 81
		 'optional_const', 0, undef
	],
	[#Rule 82
		 'optional_const', 1,
sub
#line 433 "idl.yp"
{ 1 }
	],
	[#Rule 83
		 'element_list2', 0, undef
	],
	[#Rule 84
		 'element_list2', 1, undef
	],
	[#Rule 85
		 'element_list2', 2,
sub
#line 441 "idl.yp"
{ [ $_[2] ] }
	],
	[#Rule 86
		 'element_list2', 4,
sub
#line 443 "idl.yp"
{ push(@{$_[1]}, $_[4]); $_[1] }
	],
	[#Rule 87
		 'array_len', 0, undef
	],
	[#Rule 88
		 'array_len', 3,
sub
#line 449 "idl.yp"
{ push(@{$_[3]}, "*"); $_[3] }
	],
	[#Rule 89
		 'array_len', 4,
sub
#line 451 "idl.yp"
{ push(@{$_[4]}, "$_[2]"); $_[4] }
	],
	[#Rule 90
		 'property_list', 0, undef
	],
	[#Rule 91
		 'property_list', 4,
sub
#line 457 "idl.yp"
{ FlattenHash([$_[1],$_[3]]); }
	],
	[#Rule 92
		 'properties', 1,
sub
#line 461 "idl.yp"
{ $_[1] }
	],
	[#Rule 93
		 'properties', 3,
sub
#line 463 "idl.yp"
{ FlattenHash([$_[1], $_[3]]); }
	],
	[#Rule 94
		 'property', 1,
sub
#line 467 "idl.yp"
{{ "$_[1]" => "1"     }}
	],
	[#Rule 95
		 'property', 4,
sub
#line 469 "idl.yp"
{{ "$_[1]" => "$_[3]" }}
	],
	[#Rule 96
		 'commalisttext', 1, undef
	],
	[#Rule 97
		 'commalisttext', 3,
sub
#line 475 "idl.yp"
{ "$_[1],$_[3]" }
	],
	[#Rule 98
		 'anytext', 0,
sub
#line 480 "idl.yp"
{ "" }
	],
	[#Rule 99
		 'anytext', 1, undef
	],
	[#Rule 100
		 'anytext', 1, undef
	],
	[#Rule 101
		 'anytext', 1, undef
	],
	[#Rule 102
		 'anytext', 3,
sub
#line 488 "idl.yp"
{ "$_[1]$_[2]$_[3]" }
	],
	[#Rule 103
		 'anytext', 3,
sub
#line 490 "idl.yp"
{ "$_[1]$_[2]$_[3]" }
	],
	[#Rule 104
		 'anytext', 3,
sub
#line 492 "idl.yp"
{ "$_[1]$_[2]$_[3]" }
	],
	[#Rule 105
		 'anytext', 3,
sub
#line 494 "idl.yp"
{ "$_[1]$_[2]$_[3]" }
	],
	[#Rule 106
		 'anytext', 3,
sub
#line 496 "idl.yp"
{ "$_[1]$_[2]$_[3]" }
	],
	[#Rule 107
		 'anytext', 3,
sub
#line 498 "idl.yp"
{ "$_[1]$_[2]$_[3]" }
	],
	[#Rule 108
		 'anytext', 3,
sub
#line 500 "idl.yp"
{ "$_[1]$_[2]$_[3]" }
	],
	[#Rule 109
		 'anytext', 3,
sub
#line 502 "idl.yp"
{ "$_[1]$_[2]$_[3]" }
	],
	[#Rule 110
		 'anytext', 3,
sub
#line 504 "idl.yp"
{ "$_[1]$_[2]$_[3]" }
	],
	[#Rule 111
		 'anytext', 3,
sub
#line 506 "idl.yp"
{ "$_[1]$_[2]$_[3]" }
	],
	[#Rule 112
		 'anytext', 3,
sub
#line 508 "idl.yp"
{ "$_[1]$_[2]$_[3]" }
	],
	[#Rule 113
		 'anytext', 3,
sub
#line 510 "idl.yp"
{ "$_[1]$_[2]$_[3]" }
	],
	[#Rule 114
		 'anytext', 3,
sub
#line 512 "idl.yp"
{ "$_[1]$_[2]$_[3]" }
	],
	[#Rule 115
		 'anytext', 5,
sub
#line 514 "idl.yp"
{ "$_[1]$_[2]$_[3]$_[4]$_[5]" }
	],
	[#Rule 116
		 'anytext', 5,
sub
#line 516 "idl.yp"
{ "$_[1]$_[2]$_[3]$_[4]$_[5]" }
	],
	[#Rule 117
		 'identifier', 1, undef
	],
	[#Rule 118
		 'optional_identifier', 0, undef
	],
	[#Rule 119
		 'optional_identifier', 1, undef
	],
	[#Rule 120
		 'constant', 1, undef
	],
	[#Rule 121
		 'text', 1,
sub
#line 534 "idl.yp"
{ "\"$_[1]\"" }
	],
	[#Rule 122
		 'optional_semicolon', 0, undef
	],
	[#Rule 123
		 'optional_semicolon', 1, undef
	]
],
                                  @_);
    bless($self,$class);
}

#line 546 "idl.yp"


use Parse::Pidl qw(error);

#####################################################################
# flatten an array of hashes into a single hash
sub FlattenHash($)
{
	my $a = shift;
	my %b;
	for my $d (@{$a}) {
		for my $k (keys %{$d}) {
		$b{$k} = $d->{$k};
		}
	}
	return \%b;
}

#####################################################################
# traverse a perl data structure removing any empty arrays or
# hashes and any hash elements that map to undef
sub CleanData($)
{
	sub CleanData($);
	my($v) = shift;

	return undef if (not defined($v));

	if (ref($v) eq "ARRAY") {
		foreach my $i (0 .. $#{$v}) {
			CleanData($v->[$i]);
		}
		# this removes any undefined elements from the array
		@{$v} = grep { defined $_ } @{$v};
	} elsif (ref($v) eq "HASH") {
		foreach my $x (keys %{$v}) {
			CleanData($v->{$x});
			if (!defined $v->{$x}) {
				delete($v->{$x});
				next;
			}
		}
	}

	return $v;
}

sub _Error {
	if (exists $_[0]->YYData->{ERRMSG}) {
		error($_[0]->YYData, $_[0]->YYData->{ERRMSG});
		delete $_[0]->YYData->{ERRMSG};
		return;
	}

	my $last_token = $_[0]->YYData->{LAST_TOKEN};

	error($_[0]->YYData, "Syntax error near '$last_token'");
}

sub _Lexer($)
{
	my($parser)=shift;

	$parser->YYData->{INPUT} or return('',undef);

again:
	$parser->YYData->{INPUT} =~ s/^[ \t]*//;

	for ($parser->YYData->{INPUT}) {
		if (/^\#/) {
			# Linemarker format is described at
			# http://gcc.gnu.org/onlinedocs/cpp/Preprocessor-Output.html
			if (s/^\# (\d+) \"(.*?)\"(( \d+){1,4}|)//) {
				$parser->YYData->{LINE} = $1-1;
				$parser->YYData->{FILE} = $2;
				goto again;
			}
			if (s/^\#line (\d+) \"(.*?)\"( \d+|)//) {
				$parser->YYData->{LINE} = $1-1;
				$parser->YYData->{FILE} = $2;
				goto again;
			}
			if (s/^(\#.*)$//m) {
				goto again;
			}
		}
		if (s/^(\n)//) {
			$parser->YYData->{LINE}++;
			goto again;
		}
		if (s/^\"(.*?)\"//) {
			$parser->YYData->{LAST_TOKEN} = $1;
			return('TEXT',$1);
		}
		if (s/^(\d+)(\W|$)/$2/) {
			$parser->YYData->{LAST_TOKEN} = $1;
			return('CONSTANT',$1);
		}
		if (s/^([\w_]+)//) {
			$parser->YYData->{LAST_TOKEN} = $1;
			if ($1 =~
			    /^(coclass|interface|import|importlib
			      |include|cpp_quote|typedef
			      |union|struct|enum|bitmap|pipe
			      |void|const|unsigned|signed)$/x) {
				return $1;
			}
			return('IDENTIFIER',$1);
		}
		if (s/^(.)//s) {
			$parser->YYData->{LAST_TOKEN} = $1;
			return($1,$1);
		}
	}
}

sub parse_string
{
	my ($data,$filename) = @_;

	my $self = new Parse::Pidl::IDL;

	$self->YYData->{FILE} = $filename;
	$self->YYData->{INPUT} = $data;
	$self->YYData->{LINE} = 0;
	$self->YYData->{LAST_TOKEN} = "NONE";

	my $idl = $self->YYParse( yylex => \&_Lexer, yyerror => \&_Error );

	return CleanData($idl);
}

sub parse_file($$)
{
	my ($filename,$incdirs) = @_;

	my $saved_delim = $/;
	undef $/;
	my $cpp = $ENV{CPP};
	my $options = "";
	if (! defined $cpp) {
		if (defined $ENV{CC}) {
			$cpp = "$ENV{CC}";
			$options = "-E";
		} else {
			$cpp = "cpp";
		}
	}
	my $includes = join('',map { " -I$_" } @$incdirs);
	my $data = `$cpp $options -D__PIDL__$includes -xc "$filename"`;
	$/ = $saved_delim;

	return parse_string($data, $filename);
}

1;
