/* main.c: Entry point for Siliko CLI
 * Copyright 2012-2025 Vincent Damewood
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined USE_UNIX
#	include <unistd.h>
#	define ISATTY() (isatty(fileno(stdin)) && isatty(fileno(stdout)))
#else
#	define ISATTY() (-1)
#endif

#if HAVE_READLINE
#include <readline/readline.h>
#include <readline/history.h>
#else
char *readline(const char *);
void add_history(const char *);
#endif /* HAVE_READLINE */

#include <SilikoCore/FunctionCaller.h>
#include <SilikoCore/InfixParser.h>
#include <SilikoCore/StringSource.h>
#include <SilikoCore/SyntaxTree.h>
#include <SilikoCore/Value.h>

int main(int argc, char *argv[])
{
	const char *prompt;
	const char *response;

	if (ISATTY())
	{
		prompt = "> ";
		response = "= ";
	}
	else
	{
		prompt = "";
		response = "";
	}

	SilikoFunctionCaller *caller = SilikoFunctionCallerNew();
	SilikoFunctionCallerInstallOperators(caller);
	SilikoFunctionCallerInstallFunctions(caller);

	char *expression = NULL;
	char *old_expression = NULL;

	while(-1)
	{
		expression = readline(prompt);

		if(!expression)
		{
			free(old_expression);
			old_expression = NULL;
			break;
		}

		if(*expression && (!old_expression || strcmp(expression, old_expression) != 0))
			add_history(expression);

		free(old_expression);
		old_expression = expression;

		SilikoSyntaxTreeNode *tree = SilikoParseInfix(
			SilikoStringSourceNew(expression));
		SilikoValue *result = SilikoSyntaxTreeEvaluate(tree, caller);
		SilikoSyntaxTreeDelete(tree);

		switch (SilikoValueGetStatus(result))
		{
		case SilikoValueInteger:
			printf("%lli\n", SilikoValueToInteger(result));
			break;
		case SilikoValueReal:
			printf("%f\n", SilikoValueToReal(result));
			break;
		case SilikoValueError:
			switch(SilikoValueToError(result))
		{
			case(SilikoErrorMemory):
				puts("Out of memory");
				break;
			case SilikoErrorSyntax:
				puts("Syntax error.");
				break;
			case SilikoErrorZeroDivision:
				puts("Division by zero");
				break;
			case SilikoErrorFunctionName:
				puts("Function not found");
				break;
			case SilikoErrorFunctionArguments:
				puts("Bad argument count");
				break;
			case SilikoErrorDomain:
				puts("Domain error");
				break;
			case SilikoErrorRange:
				puts("Range error");
				break;
			default:
				puts("Unexpected error");
			}
		}
		SilikoValueDelete(result);
	}

	SilikoFunctionCallerDelete(caller);

	if (ISATTY())
	{
		fputc('\n', stdout);
		fflush(stdout);
	}
	return 0;
}
