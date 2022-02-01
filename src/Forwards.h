#pragma once

class BoxedValue;
enum OperatorType;

namespace ast
{
	class ASTNode;
}

namespace parse
{
	class ParserBase;
	class StaticString;
	class Alphabet;
}

namespace runtime
{
	class Stack;
	class DispatchEngine;
}

namespace bindings
{
	//class IFunctionBinding;
}