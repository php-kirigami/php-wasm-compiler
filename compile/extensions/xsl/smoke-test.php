<?php
// Exercised by compile/check-shared-extension-symbols.mjs. A real
// transformation: the stylesheet and source are ext/dom documents (the
// core's static dom + libxml), handed to libxslt inside xsl.so. It also
// covers an EXSLT function (libexslt) and a PHP callback
// (registerPHPFunctions, which calls back into the core's dom xpath
// callback code).

function fail(string $message): never {
	echo $message, "\n";
	exit(1);
}

if (!extension_loaded('xsl')) {
	fail('xsl not loaded');
}
if (!(new XSLTProcessor())->hasExsltSupport()) {
	fail('XSLTProcessor::hasExsltSupport() is false (libexslt not linked in)');
}

$xml = new DOMDocument();
$xml->loadXML('<items><item>b</item><item>a</item><item>c</item></items>');

$xsl = new DOMDocument();
$xsl->loadXML(<<<'XSL'
<xsl:stylesheet version="1.0"
	xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
	xmlns:str="http://exslt.org/strings"
	xmlns:php="http://php.net/xsl"
	extension-element-prefixes="str">
	<xsl:output method="text"/>
	<xsl:template match="/">
		<xsl:for-each select="items/item">
			<xsl:sort select="."/>
			<xsl:value-of select="php:function('strtoupper', string(.))"/>
		</xsl:for-each>
		<xsl:text>|</xsl:text>
		<xsl:value-of select="count(str:tokenize('x,y,z', ','))"/>
	</xsl:template>
</xsl:stylesheet>
XSL);

$proc = new XSLTProcessor();
$proc->registerPHPFunctions('strtoupper');
if (!$proc->importStylesheet($xsl)) {
	fail('importStylesheet() failed');
}

$out = $proc->transformToXml($xml);
if ($out !== 'ABC|3') {
	fail('transformToXml() returned ' . var_export($out, true) . ", expected 'ABC|3'");
}

// The result as a DOM document: created by xsl.so's libxslt, then owned
// (and eventually freed) by the core's ext/dom.
$doc = $proc->transformToDoc($xml);
if (!$doc instanceof DOMDocument) {
	fail('transformToDoc() did not return a DOMDocument');
}
unset($doc);

echo "xsl smoke test OK\n";
