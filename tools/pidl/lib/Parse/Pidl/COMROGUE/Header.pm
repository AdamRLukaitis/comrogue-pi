# COM Header generation
# (C) 2005 Jelmer Vernooij <jelmer@samba.org>
# Modifications (C) 2013 Eric J. Bowersox <erbo@erbosoft.com>

package Parse::Pidl::COMROGUE::Header;

use Parse::Pidl::Typelist qw(mapTypeName maybeMapScalarType is_struct);
use Parse::Pidl::Util qw(has_property is_constant);

use vars qw($VERSION);
$VERSION = '0.01';

use strict;

sub stripquotes($)
{
    my $str = shift;
    $str =~ s/^\"//;
    $str =~ s/\"$//;
    return $str;
}

sub GetArgumentProtoList($)
{
    my $f = shift;
    my $res = "";
    my $first = 1;

    foreach my $a (@{$f->{ELEMENTS}}) {
	$res .= ", " unless $first;
	$first = 0;
	$res .= maybeMapScalarType($a->{TYPE}) . " ";

	my $l = $a->{POINTERS};
	$l-- if (Parse::Pidl::Typelist::scalar_is_reference($a->{TYPE}));
	foreach my $i (1..$l) {
	    $res .= "*";
	}

	if (defined $a->{ARRAY_LEN}[0] && !is_constant($a->{ARRAY_LEN}[0]) &&
	!$a->{POINTERS}) {
	    $res .= "*";
	}
	$res .= $a->{NAME};
	if (defined $a->{ARRAY_LEN}[0] && is_constant($a->{ARRAY_LEN}[0])) {
	    $res .= "[$a->{ARRAY_LEN}[0]]";
	}
    }

    return undef if $first;
    return $res;
}

sub GetArgumentList($)
{
    my $f = shift;
    my $res = "";
    my $first = 1;

    foreach (@{$f->{ELEMENTS}}) {
	$res .= ", " unless $first;
	$first = 0;
	$res .= "$_->{NAME}";
    }

    return undef if $first;
    return $res;
}

sub MethodsDefinition($)
{
    my $interface = shift;
    my $res = "";
    
    $res .= "#define METHODS_" . $interface->{NAME} . " \\\n";
    if (defined($interface->{BASE})) {
	$res .= "\tINHERIT_METHODS(METHODS_" . $interface->{BASE} . ") \\\n";
    }

    my $data = $interface->{DATA};
    foreach my $d (@{$data}) {
	next unless ($d->{TYPE} eq "FUNCTION");
	if ($d->{RETURN_TYPE} eq "HRESULT") {
	    $res .= "\tSTDMETHOD($d->{NAME})";
	} else {
	    $res .= "\tSTDMETHOD_($d->{NAME}," . $d->{RETURN_TYPE} . ")";
	}
	my $args = GetArgumentProtoList($d);
	if (defined($args)) {
	    $res .= "(THIS_($interface->{NAME}) $args) PURE;\\\n";
	} else {
	    $res .= "(THIS($interface->{NAME})) PURE;\\\n";
	}
    }
    $res .= "\tEND_METHODS\n\n";
    return $res;
}

sub MakeGUIDDef($$$)
{
    my $t = shift;
    my $name = shift;
    my $uuid = shift;

    my @uuidparts = split(/-/, $uuid);
    my $b1 = substr($uuidparts[3], 0, 2);
    my $b2 = substr($uuidparts[3], 2, 2);
    my $b3 = substr($uuidparts[4], 0, 2);
    my $b4 = substr($uuidparts[4], 2, 2);
    my $b5 = substr($uuidparts[4], 4, 2);
    my $b6 = substr($uuidparts[4], 6, 2);
    my $b7 = substr($uuidparts[4], 8, 2);
    my $b8 = substr($uuidparts[4], 10, 2);

    return "DEFINE_$t(${t}_$name, 0x$uuidparts[0], 0x$uuidparts[1], 0x$uuidparts[2], " .
	        "0x$b1, 0x$b2, 0x$b3, 0x$b4, 0x$b5, 0x$b6, 0x$b7, 0x$b8);\n\n";
}

sub ParseImports($)
{
    my $imp = shift;
    my $res = "";
    my $seen = 0;

    foreach my $p (@{$imp->{PATHS}}) {
	my $header = $p;
	$header =~ s/\.idl/\.h/;
	$header =~ s/^\"/</;
	$header =~ s/\"$/>/;
	$res .= "#include $header\n";
	$seen = 1;
    }
    $res .= "\n" if $seen;
    return $res;
}

sub ParseElement($$)
{
    my $prefix = shift;
    my $element = shift;
    my $res = "";

    $res .= $prefix . maybeMapScalarType($element->{TYPE}) . " " . $element->{NAME};
    if (defined($element->{ARRAY_LEN})) {
	foreach my $l (@{$element->{ARRAY_LEN}}) {
	    $res .= "[" . $l . "]";
	}
    }
    $res .= ";\n";
    return $res;
}

sub ParseTypedef($)
{
    my $def = shift;
    my $res = "";

    $res .= "typedef ";
    $res .= "const " if ($def->{CONST});
    if (ref($def->{DATA}) ne "HASH") {
	$res .= maybeMapScalarType($def->{DATA}) . " ";
    } else {
	if (is_struct($def->{DATA})) {
	    $res .= mapTypeName($def->{DATA}) . " {\n";
	    foreach my $elt (@{$def->{DATA}->{ELEMENTS}}) {
		$res .= ParseElement("\t", $elt);
	    }
	    $res .= "} ";
	} else {
	    $res .= mapTypeName($def->{DATA}) . " ";
	}
    }
    my $l = $def->{POINTERS};
    $l-- if (Parse::Pidl::Typelist::scalar_is_reference($def->{TYPE}));
    foreach my $i (1..$l) {
	$res .= "*";
    }
    $res .= $def->{NAME} . ";\n";
    return $res;
}

sub ParseTypedefs($)
{
    my $if = shift;
    my $res = "";
    my $count = 0;

    foreach my $d (@{$if->{DATA}}) {
	$res .= stripquotes($d->{DATA}) . "\n" if ($d->{TYPE} eq "CPP_QUOTE");
	next unless ($d->{TYPE} eq "TYPEDEF");
	++$count;
	$res .= ParseTypedef($d);
    }

    $res .= "\n";
    return "" if ($count == 0);
    return $res;
}

sub ParseInterface($)
{
    my $if = shift;
    my $res;
    my $d;

    $res .= "/*---------------------------------------------------------------\n";
    $res .= " * Interface $if->{NAME}\n";
    $res .= " *---------------------------------------------------------------\n";
    $res .= " */\n\n";

    $res .= MakeGUIDDef("IID", $if->{NAME}, $if->{PROPERTIES}->{uuid});

    $res .= MethodsDefinition($if);

    if (defined($if->{BASE})) {
	$res .= "BEGIN_INTERFACE_(" . $if->{NAME} . ", " . $if->{BASE} . ")\n";
    } else {
	$res .= "BEGIN_INTERFACE(" . $if->{NAME} . ")\n";
    }
    $res .= "\tMETHODS_" . $if->{NAME} . "\n";
    $res .= "END_INTERFACE(" . $if->{NAME} . ")\n\n";

    foreach $d (@{$if->{DATA}}) {
	$res .= stripquotes($d->{DATA}) . "\n" if ($d->{TYPE} eq "CPP_QUOTE");
	$res .= ParseTypedef($d) if ($d->{TYPE} eq "TYPEDEF");
    }
    
    $res .= "\n#ifdef CINTERFACE\n\n";
    
    foreach $d (@{$if->{DATA}}) {
	next unless ($d->{TYPE} eq "FUNCTION");
	my $args = GetArgumentList($d);
	if (defined($args)) {
	    $res .= "#define $if->{NAME}_$d->{NAME}(pInterface, $args) \\\n";
	    $res .= "\t(*((pInterface)->pVTable->$d->{NAME}))(($if->{NAME} *)(pInterface), $args)\n";
	} else {
	    $res .= "#define $if->{NAME}_$d->{NAME}(pInterface) \\\n";
	    $res .= "\t(*((pInterface)->pVTable->$d->{NAME}))(($if->{NAME} *)(pInterface))\n";
	}
    }

    $res .= "\n#endif  /* CINTERFACE */\n\n";
    return $res;
}

sub ParseCoClass($)
{
    my ($c) = @_;
    my $res = "";

    $res .= "/*---------------------------------------------------------------\n";
    $res .= " * Class $c->{NAME}\n";
    $res .= " *---------------------------------------------------------------\n";
    $res .= " */\n\n";

    $res .= MakeGUIDDef("CLSID", $c->{NAME}, $c->{PROPERTIES}->{uuid});

    if (has_property($c, "progid")) {
	$res .= "#define PROGID_" . $c->{NAME} . " \"$c->{PROPERTIES}->{progid}\"\n";
    }
    $res .= "\n";
    return $res;
}

sub Parse($$$)
{
    my ($idl,$basename, $srcfile) = @_;
    my $res = "";
    my $has_obj = 0;

    $res .= "/* COMROGUE: Autogenerated from IDL file $srcfile */\n\n";

    my $include_sym = "__" . uc($basename) . "_H_INCLUDED";

    $res .= "#ifndef $include_sym\n" .
	"#define $include_sym\n\n" .
	"#ifndef __ASM__\n\n";
    my $want_macro_headers = 1;

    foreach (@{$idl})
    {
	if ($_->{TYPE} eq "CPP_QUOTE") {
	    $res .= stripquotes($_->{DATA}) . "\n";
	}
	if ($_->{TYPE} eq "IMPORT") {
	    $res .= ParseImports($_);
	}

	if ($_->{TYPE} eq "INTERFACE") {
	    if (has_property($_, "object")) {
		if ($want_macro_headers) {
		    $res .= "#include <comrogue/object_definition_macros.h>\n\n";
		    $want_macro_headers = 0;
		}
		$res .= ParseInterface($_);
	    } else {
		$res .= ParseTypedefs($_);
	    }
	    $has_obj = 1;
	} 

	if ($_->{TYPE} eq "COCLASS") {
	    if ($want_macro_headers) {
		$res .= "#include <comrogue/object_definition_macros.h>\n\n";
		$want_macro_headers = 0;
	    }
	    $res.=ParseCoClass($_);
	    $has_obj = 1;
	}
    }

    $res .= "#endif  /* __ASM__ */\n\n";
    $res .= "#endif  /* $include_sym */\n";
    
    return $res if ($has_obj);
    return undef;
}

1;
