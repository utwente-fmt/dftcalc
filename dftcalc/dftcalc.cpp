/*
 * dftcalc.cpp
 * 
 * Part of dft2lnt library - a library containing read/write operations for DFT
 * files in Galileo format and translating DFT specifications into Lotos NT.
 * 
 * @author Freark van der Berg, extended by Dennis Guck,
 * Axel Belinfante, and Enno Ruijters
 */

#include <vector>
#include <string>
#include <functional>
#include <unordered_map>
#include <stdlib.h>
#include <cstdio>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <iostream>
#include <fstream>
#include <memory>
#include <limits.h>

#ifdef HAVE_CADP
# include <CADP.h>
#endif

#ifdef WIN32
#include <io.h>
#endif

using namespace std;

#include "Shell.h"
#include "FileSystem.h"
#include "MessageFormatter.h"
#include "dftcalc.h"
#include "dft2lnt.h"
#include "compiletime.h"
#include "yaml-cpp/yaml.h"
#include "mrmc.h"
#include "modest.h"
#include "imca.h"
#include "storm.h"

const int DFT::DFTCalc::VERBOSITY_SEARCHING = 2;

const int DFT::VERBOSITY_FLOW = 1;
const int VERBOSITY_DATA = 1;

void print_help(MessageFormatter* messageFormatter, string topic="") {
	if(topic.empty()) {
		messageFormatter->notify ("dftcalc [INPUTFILE.dft] [options]");
		messageFormatter->message("  Calculates the failure probability for the specified DFT file, given the");
		messageFormatter->message("  specified time constraints. Result is written to the specified output file.");
		messageFormatter->message("  Check dftcalc --help=output for more details regarding the output.");
		messageFormatter->message("");
		messageFormatter->notify ("General Options:");
		messageFormatter->message("  -h, --help      Show this help.");
		messageFormatter->message("  --color         Use colored messages.");
		messageFormatter->message("  --no-color      Do not use colored messages.");
		messageFormatter->message("  --version       Print version info and quit.");
		messageFormatter->message("  -R              Reuse existing output files.");
		messageFormatter->message("  -M              Use modularization to check static parts of DFT.");
		messageFormatter->message("  --storm         Use Storm. (standard setting)");
		messageFormatter->message("  --modest        Use Modest instead of Storm.");
#ifdef HAVE_CADP
		messageFormatter->message("  --mrmc          Use MRMC instead of Storm.");
		messageFormatter->message("  --imca          Use IMCA instead of MRMC.");
#endif
		messageFormatter->message("  --imrmc         Use IMRMC instead of Storm.");
		messageFormatter->message("  --exact         Use DFTRES to give (more) exact results.");
		messageFormatter->message("  --no-nd-warning Do not warn (but give notice) for non-determinism.");
		messageFormatter->message("");
		messageFormatter->notify ("Debug Options:");
		messageFormatter->message("  --verbose=x     Set verbosity to x, -1 <= x <= 5.");
		messageFormatter->message("  -v, --verbose   Increase verbosity. Up to 5 levels.");
		messageFormatter->message("  -q              Decrease verbosity.");
		messageFormatter->message("");
		messageFormatter->notify ("Output Options:");
		messageFormatter->message("  -r FILE         Output result in YAML format to this file. (see --help=output)");
		messageFormatter->message("  -c FILE         Output result as CSV format to this file. (see --help=output)");
		messageFormatter->message("  -p              Print result to stdout.");
		messageFormatter->message("  -e evidence     Comma separated list of BE names that fail at startup.");
		messageFormatter->message("  -m              Calculate mean time to failure (requires --storm or --imca).");
		messageFormatter->message("                  Overrules -i, -t, -u, -f, --mrmc, --imrmc.");
		messageFormatter->message("  -s              Calculate P(DFT failed) at steady-state (requires --storm or --imrmc).");
		messageFormatter->message("  -i l u s        Calculate P(DFT fails in [0,x] time units) for each x in interval,");
		messageFormatter->message("                  where interval is given by [l .. u] with step s ");
		messageFormatter->message("  -t xList        Calculate P(DFT fails in [0,x] time units) for each x in xList,");
		messageFormatter->message("                  where xList is a whitespace-separated list of values, default is \"1\"");
		messageFormatter->message("  -u              Calculate P(DFT fails eventually)");
		messageFormatter->message("  -I l u          Calculate P(DFT fails in [l,u] time units) where l can be >= 0");
		messageFormatter->message("  -f <command>    Raw Calculation formula for the model-checker. Overrules -i, -s, -m, -t, and -u.");
		messageFormatter->message("  -E errorbound   Error bound, to be passed to IMCA.");
		messageFormatter->message("  -C DIR          Temporary output files will be in this directory");
		messageFormatter->message("  --min           Compute minimum time-bounded reachability (default)");
		messageFormatter->message("  --max           Compute maximum time-bounded reachability");
		messageFormatter->flush();
	} else if(topic=="output") {
		messageFormatter->notify ("Output");
		messageFormatter->message("  The output file specified with -r uses YAML syntax.");
		messageFormatter->message("  The top node is a map, containing one element, a mapping containing various");
		messageFormatter->message("  information regarding the DFT. E.g. it looks like this:");
		messageFormatter->message("  b.dft:");
		messageFormatter->message("    dft: b.dft");
		messageFormatter->message("    failProb: 0.3934693");
		messageFormatter->message("    stats:");
		messageFormatter->message("      time_user: 0.54");
		messageFormatter->message("      time_system: 0.21");
		messageFormatter->message("      time_elapsed: 1.8");
		messageFormatter->message("      mem_virtual: 13668");
		messageFormatter->message("      mem_resident: 1752");
		messageFormatter->message("  The Calculation command can be manually set using -f.");
		messageFormatter->message("  For Storm the defaults is:");
		messageFormatter->message("    Pmax=? [F<=n failed=true ]             (default)");
		messageFormatter->message("  For MRMC the defaults are:");
		messageFormatter->message("    P{>1} [ tt U[0,n] reach ]             (default)");
		messageFormatter->message("    P{<0} [ tt U[0,n] reach ]             (when --max is given)");
		messageFormatter->message("  and for IMCA the defaults are:");
		messageFormatter->message("    -min -tb -T n                         (default)");
		messageFormatter->message("    -max -tb -T n                         (when --max is given)");
		messageFormatter->message("  where n is the mission time (specified via -t or -i), default is 1.");
	} else if(topic=="topics") {
		messageFormatter->notify ("Help topics:");
		messageFormatter->message("  output          Displays the specification of the output format");
		messageFormatter->message("  To view topics: dftcalc --help=<topic>");
		messageFormatter->message("");
	} else {
		messageFormatter->reportAction("Unknown help topic: " + topic);
	}		
}

void print_help_output(MessageFormatter* messageFormatter) {
}

void print_version(MessageFormatter* messageFormatter) {
	messageFormatter->notify ("dftcalc");
	messageFormatter->message(string("  built on ") + COMPILETIME_DATE);
	{
		FileWriter out;
		std::string version = COMPILETIME_GITVERSION;
		out << string("  git version: ") << version;
		if (version.back() == '+' || COMPILETIME_GITCHANGED)
			out << " (nearest)";
		out << out.applypostfix;
		out << string("  git revision `") + COMPILETIME_GITREV + "'";
		if(COMPILETIME_GITCHANGED)
			out << " + uncommited changes";
		messageFormatter->message(out.toString());
	}
	messageFormatter->message("  ** Copyright statement. **");
	messageFormatter->flush();
}

File DFT::DFTCalc::getDftresJar() {
	char* root = getenv((const char*)"DFTRES");
	std::string dftresRoot = root?string(root):"";

	if(dftresRoot == "") {
		File ret("DFTRES.jar");
		if(!FileSystem::exists(ret)) {
			if(messageFormatter)
				messageFormatter->reportError("Environment variable `DFTRES' not set. Please set it to where DFTRES.jar can be found.");
		}
		return ret;
	}

	File ret(dftresRoot + "/DFTRES.jar");
	if(!FileSystem::exists(ret)) {
		if(messageFormatter)
			messageFormatter->reportError("DFTRES.jar not found at " + ret.getFileRealPath());
	}
	return ret;
}

std::string DFT::DFTCalc::getCoralRoot() {
	
	if(!coralRoot.empty()) {
		return coralRoot;
	}
	
	char* root = getenv("CORAL");
	std::string coralRoot = root?string(root):"";
	
	if(coralRoot=="") {
		if(messageFormatter) messageFormatter->reportError("Environment variable `CORAL' not set. Please set it to where coral can be found.");
		goto end;
	}

	for (int i = coralRoot.length() - 1; i >= 0; i--) {
		if (coralRoot[i] == '\\')
			coralRoot[i] = '/';
	}
	if (coralRoot[coralRoot.length() - 1] == '/')
		coralRoot = coralRoot.substr(0, coralRoot.length() - 1);
end:
	return coralRoot;
}

std::string DFT::DFTCalc::getImcaRoot() {
	
	if(!imcaRoot.empty()) {
		return imcaRoot;
	}
	
	char* root = getenv("IMCA");
	std::string imcaRoot = root?string(root):"";
	if(imcaRoot=="") {
		if(messageFormatter) messageFormatter->reportError("Environment variable `IMCA' not set. Please set it to where IMCA can be found.");
		goto end;
	}
	
	for (int i = imcaRoot.length() - 1; i >= 0; i--) {
		if (imcaRoot[i] == '\\')
			imcaRoot[i] = '/';
	}
	if (imcaRoot[imcaRoot.length() - 1] == '/')
		imcaRoot = imcaRoot.substr(0, imcaRoot.length() - 1);
end:
	return imcaRoot;
}

std::string DFT::DFTCalc::getRoot() {
	
	if(!dft2lntRoot.empty()) {
		return dft2lntRoot;
	}
	
	char* root = getenv((const char*)"DFT2LNTROOT");
	std::string dft2lntRoot = root?string(root):"";
	
	if(dft2lntRoot=="")
		dft2lntRoot = DFT2LNTROOT;
	
	for (int i = dft2lntRoot.length() - 1; i >= 0; i--) {
		if (dft2lntRoot[i] == '\\')
			dft2lntRoot[i] = '/';
	}
	if (dft2lntRoot[dft2lntRoot.length() - 1] == '/')
		dft2lntRoot = dft2lntRoot.substr(0, dft2lntRoot.length() - 1);

	if (!FileSystem::isDir(dft2lntRoot)) {
		if (messageFormatter)
			messageFormatter->reportError("DFT2LNTROOT does not exist or is not a directory: " + dft2lntRoot);
	}

#ifdef HAVE_CADP
	struct stat rootStat;
	if(stat((dft2lntRoot+DFT2LNT::LNTSUBROOT).c_str(),&rootStat)) {
		if(FileSystem::mkdir(dft2lntRoot+DFT2LNT::LNTSUBROOT,0755)) {
			if(messageFormatter) messageFormatter->reportError("Could not create LNT Nodes directory (`" + dft2lntRoot+DFT2LNT::LNTSUBROOT + "')");
			dft2lntRoot = "";
			goto end;
		}
	}

	if(stat((dft2lntRoot+DFT2LNT::BCGSUBROOT).c_str(),&rootStat)) {
		if(FileSystem::mkdir(dft2lntRoot+DFT2LNT::BCGSUBROOT,0755)) {
			if(messageFormatter) messageFormatter->reportError("Could not create BCG Nodes directory (`" + dft2lntRoot+DFT2LNT::BCGSUBROOT + "')");
			dft2lntRoot = "";
			goto end;
		}
	}
#endif

	if(messageFormatter) messageFormatter->reportAction("DFT2LNTROOT is: " + dft2lntRoot,VERBOSITY_DATA);
end:
	return dft2lntRoot;
}

#ifdef HAVE_CADP
std::string DFT::DFTCalc::getCADPRoot() {
	string cadp = CADP::getRoot();

	return cadp;
}

static bool hasHiddenLabels(const File& file) {
	std::string* fileContents = FileSystem::load(file);
	bool res = false;
	if (fileContents) {
		res = ((*fileContents).find("no transition with a hidden label", 0) ==  string::npos);
		delete fileContents;
	}
	return res;
}

static bool hasImpossibleLabel(const File& file) {
	std::string* fileContents = FileSystem::load(file);
	bool res = false;
	if (fileContents) {
		res = ((*fileContents).find("IMPOSSIBLE", 0) != string::npos);
		delete fileContents;
	}
	return res;
}
#endif

int isReal(string s, double *res) {
	char *end;
	*res = strtod(s.c_str(), &end);
	if (*end)
		return 0;
	return 1;
}

int DFT::DFTCalc::calcModular(const bool reuse,
                              const std::string& cwd,
                              const File& dftOriginal,
                              const std::vector<Query> &queries,
                              enum DFT::checker useChecker,
                              enum DFT::converter useConverter,
                              bool warnNonDeterminism,
                              DFT::DFTCalculationResult &ret,
                              bool expOnly,
                              bool exactMode)
{
	std::string dftFileName = dftOriginal.getFileBase();
	messageFormatter->notify("Computing `"+dftFileName+"' with modularization");
	CommandExecutor exec(messageFormatter, cwd, dftFileName);

	File mod = dftOriginal.newWithPathTo(cwd).newWithExtension("mod");

	if(!reuse) {
		if(FileSystem::exists(mod))    FileSystem::remove(mod);
	} else {
		messageFormatter->reportAction("Preserving generated modules file.",
		                               VERBOSITY_FLOW);
	}
	if(!reuse || !FileSystem::exists(mod)) {
		messageFormatter->reportAction("Modularizing DFT...",VERBOSITY_FLOW);
		std::vector<std::string> arguments;
		arguments.push_back("--verbose=" + std::to_string(messageFormatter->getVerbosity()));
		arguments.push_back("-m");
		arguments.push_back(mod.getFileRealPath());
		arguments.push_back(dftOriginal.getFileRealPath());
		if (exec.runCommand(dft2lntcExec.getFilePath(), arguments, "dft2lntc", mod) == "")
			return 1;
	} else {
		messageFormatter->reportAction("Reusing modules file",VERBOSITY_FLOW);
	}
	std::string* tmp = FileSystem::load(mod);
	if (!tmp) {
		messageFormatter->reportError("Error loading modules file");
		return 1;
	}
	std::string modules;
	std::swap(modules, *tmp);
	delete tmp;
	return checkModule(reuse, cwd, dftOriginal, queries, useChecker,
	                   useConverter, warnNonDeterminism, ret, expOnly, exactMode, modules);
}

static void addVoteResults(DFT::DFTCalculationResultItem &ret,
                           std::vector<DFT::DFTCalculationResultItem> P,
			   size_t votCount)
{
	if (votCount == 0) {
		ret.lowerBound = ret.upperBound = (uintmax_t)1;
		return;
	} else if (votCount > P.size()) {
		ret.lowerBound = ret.upperBound = (uintmax_t)0;
		return;
	}
	DFT::DFTCalculationResultItem cur = P[0];
	P.erase(P.begin());
	/* P{k/N}(X1, X2, ..., XN)
	 * = P(X1)*P{(k-1)/(N-1)}(X2, ..., XN)
	 *   + (1-P(X1))*P{k/(N-1)}(X2, ..., XN)
	 */
	addVoteResults(ret, P, votCount - 1);
	decnumber<> loSum = cur.lowerBound * ret.lowerBound;
	decnumber<> upSum = cur.upperBound * ret.upperBound;
	addVoteResults(ret, P, votCount);
	/* In the next part we swap the lower and upper bounds of the
	 * current entry due to the subtraction.
	 */
	loSum += (decnumber<>(1) - cur.upperBound) * ret.lowerBound;
	upSum += (decnumber<>(1) - cur.lowerBound) * ret.upperBound;
	ret.lowerBound = loSum;
	ret.upperBound = upSum;
}

static void addVoteResults(std::vector<DFT::DFTCalculationResultItem> &ret,
                           const std::vector<DFT::DFTCalculationResult> P,
			   MessageFormatter &mf,
                           size_t votCount)
{
	if (P.empty())
		return;
	size_t nResults = P[0].failProbs.size();
	for (const DFT::DFTCalculationResult r : P) {
		if (r.failProbs.size() != nResults) {
			mf.reportError("Unequal number of results for modules.");
		}
	}
	for (size_t i = 0; i < nResults; i++) {
		std::vector<DFT::DFTCalculationResultItem> probs;
		DFT::DFTCalculationResultItem ref = P[0].failProbs[i];
		for (const DFT::DFTCalculationResult r : P) {
			DFT::DFTCalculationResultItem it = r.failProbs[i];
			if (it.missionTime != ref.missionTime
			    || it.mrmcCommand != ref.mrmcCommand)
			{
				mf.reportError("Unequal order/type of results for modules.");
				return;
			}
			probs.push_back(it);
		}
		addVoteResults(ref, probs, votCount);
		ret.push_back(ref);
	}
}

int DFT::DFTCalc::checkModule(const bool reuse,
                              const std::string& cwd,
                              const File& dft,
                              const std::vector<Query> &queries,
                              enum DFT::checker useChecker,
                              enum DFT::converter useConverter,
                              bool warnNonDeterminism,
                              DFT::DFTCalculationResult &ret,
                              bool expOnly,
                              bool exactMode,
                              std::string &module)
{
	if (module.length() == 0) {
		messageFormatter->reportError("Modules file empty.");
		return 1;
	}
	size_t eol = module.find('\n');
	if (eol == std::string::npos)
		eol = module.length();
	if (module[0] == 'M') {
		std::string root = module.substr(1, eol - 1);
		module = module.substr(eol + 1);
		return calculateDFT(reuse, cwd, dft, queries, useChecker,
		                    useConverter, warnNonDeterminism, root, ret,
		                    expOnly, exactMode);
	} else if (module[0] == '=') {
		decnumber<> val(module.substr(1, eol - 1));
		module = module.substr(eol + 1);
		std::vector<Query> tmp = queries;
		expandRangeQueries(tmp);
		for (Query q : tmp) {
			DFT::DFTCalculationResultItem it(q);
			it.exactBounds = true;
			it.lowerBound = it.upperBound = val;
			ret.failProbs.push_back(it);
		}
		return 0;
	}
	char op = module[0];
	std::string numStr;
	unsigned long votCount = 0;
	if (op != '/') {
		numStr = module.substr(1, eol);
	} else {
		size_t sp = module.find(' ');
		if (sp == std::string::npos) {
			messageFormatter->reportError("Improperly formatted modules file: voting gate without child count.");
			return 1;
		}
		numStr = module.substr(sp + 1, eol);
		votCount = std::stoul(module.substr(1, sp));
	}
	module = module.substr(eol + 1);
	unsigned long num = std::stoul(numStr);
	std::vector<DFT::DFTCalculationResultItem> gather;
	std::vector<DFT::DFTCalculationResult> votResults;
	for (unsigned long i = 0; i < num; i++) {
		DFT::DFTCalculationResult tmp;
		if (module.empty()) {
			messageFormatter->reportError("Modules contains too few entries for module.");
			return 1;
		}
		if (checkModule(reuse, cwd, dft, queries, useChecker, useConverter,
		                warnNonDeterminism, tmp, expOnly, exactMode, module))
			return 1;
		if (op == '/') {
			votResults.push_back(tmp);
		} else if (i == 0) {
			gather = tmp.failProbs;
		} else {
			if (gather.size() != tmp.failProbs.size()) {
				messageFormatter->reportError("Unequal number of results for modules.");
				return 1;
			}
			for (size_t i = gather.size() - 1; i != SIZE_MAX; i--) {
				DFT::DFTCalculationResultItem &old = gather[i];
				DFT::DFTCalculationResultItem add = tmp.failProbs[i];
				if (old.missionTime != add.missionTime
				    || old.mrmcCommand != add.mrmcCommand)
				{
					messageFormatter->reportError("Unequal order/type of results for modules.");
					return 1;
				}
				if (op == '*') {
					old.lowerBound *= add.lowerBound;
					old.upperBound *= add.upperBound;
				} else if (op == '+') {
					decnumber<> one(1);
					old.lowerBound = (one - (one - old.lowerBound)
					                      * (one - add.lowerBound));
					old.upperBound = (one - (one - old.upperBound)
					                      * (one - add.upperBound));
				} else {
					messageFormatter->reportError(std::string("Unknown module type: ") + op);
					return 1;
				}
			}
		}
	}
	if (op != '/') {
		ret.failProbs.insert(ret.failProbs.begin(), gather.begin(), gather.end());
	} else {
		addVoteResults(ret.failProbs, votResults, *messageFormatter, votCount);
	}
	return 0;
}

int DFT::DFTCalc::calculateDFT(const bool reuse,
                               const std::string& cwd,
                               const File& dftOriginal,
                               const std::vector<Query> &queries,
                               enum DFT::checker useChecker,
                               enum DFT::converter useConverter,
                               bool warnNonDeterminism,
							   std::string root,
                               DFT::DFTCalculationResult &ret,
                               bool expOnly,
                               bool exactMode)
{
	File dft    = dftOriginal.newWithPathTo(cwd);
	std::string dftFileName = dft.getFileBase();
	if (!root.empty()) {
		dftFileName += "@" + root;
		dft = File(cwd, dftFileName, "dft");
	}
	File svl    = dft.newWithExtension("svl");
	File svlLog = dft.newWithExtension("log");
	File exp    = dft.newWithExtension("exp");
	File bcg    = dft.newWithExtension("bcg");
	File aut    = dft.newWithExtension("aut");
	File imc    = dft.newWithExtension("imc");
	File ctmdpi = dft.newWithExtension("ctmdpi");
	File tra = dft.newWithExtension("tra");
	File exactTra = dft.newWithExtension("exact.tra");
	File exactLab = dft.newWithExtension("exact.lab");
	File ma     = dft.newWithExtension("ma");
	File jani     = dft.newWithExtension("jani");
	File lab    = ctmdpi.newWithExtension("lab");
	File dot    = dft.newWithExtension("dot");
	File png    = dot.newWithExtension("png");
	File input  = dft.newWithExtension("input");
	File inputImca  = dft.newWithExtension("inputImca");

	Shell::RunStatistics stats;

	FileSystem::mkdir(File(cwd));

	if(!reuse) {
		//messageFormatter->notify("Deleting generated files");
		if(FileSystem::exists(dft))    FileSystem::remove(dft);
		if(FileSystem::exists(svl))    FileSystem::remove(svl);
		if(FileSystem::exists(svlLog)) FileSystem::remove(svlLog);
		if(FileSystem::exists(exp))    FileSystem::remove(exp);
		if(FileSystem::exists(bcg))    FileSystem::remove(bcg);
		if(FileSystem::exists(imc))    FileSystem::remove(imc);
		if(FileSystem::exists(tra)) FileSystem::remove(tra);
		if(FileSystem::exists(ctmdpi)) FileSystem::remove(ctmdpi);
		if(FileSystem::exists(ma))     FileSystem::remove(ma);
		if(FileSystem::exists(lab))    FileSystem::remove(lab);
		if(FileSystem::exists(dot))    FileSystem::remove(dot);
		if(FileSystem::exists(png))    FileSystem::remove(png);
		if(FileSystem::exists(input))  FileSystem::remove(input);
	} else {
		messageFormatter->reportAction("Preserving generated files",VERBOSITY_FLOW);
	}

	CommandExecutor exec(messageFormatter, cwd, dftFileName);
	this->exec = &exec;

	if(!reuse || !FileSystem::exists(dft)) {
		messageFormatter->reportAction("Canonicalizing DFT...",VERBOSITY_FLOW);
		std::vector<std::string> arguments;
		arguments.push_back("--verbose=" + std::to_string(messageFormatter->getVerbosity()));
		arguments.push_back("-t");
		arguments.push_back(dft.getFileRealPath());
		if (!root.empty()) {
			arguments.push_back("-r");
			arguments.push_back(root);
		}
		arguments.push_back(dftOriginal.getFileRealPath());
		if (exec.runCommand(dft2lntcExec.getFilePath(), arguments, "dft2lntc", dft) == "")
			return 1;
	} else {
		messageFormatter->reportAction("Reusing copy of original dft file",VERBOSITY_FLOW);
	}

	if (root == "")
		messageFormatter->notify("Calculating `"+dftFileName+"'");

	if(!reuse || !FileSystem::exists(exp) || !FileSystem::exists(svl)) {
		// dft -> exp, svl
		messageFormatter->reportAction("Translating DFT to EXP...",VERBOSITY_FLOW);
		std::vector<std::string> arguments;
		arguments.push_back("--verbose=" + std::to_string(messageFormatter->getVerbosity()));
		arguments.push_back("-s");
		arguments.push_back(svl.getFileRealPath());
		arguments.push_back("-x");
		arguments.push_back(exp.getFileRealPath());
		arguments.push_back("-b");
		arguments.push_back(bcg.getFileRealPath());
		arguments.push_back("-n");
		arguments.push_back(dftOriginal.getFileRealPath());
		if(!evidence.empty()) {
			arguments.push_back("-e");
			std::stringstream ss;
			for(std::string e: evidence) {
				ss << e << ",";
			}
			arguments.push_back(ss.str());
		}
		if (!messageFormatter->usingColoredMessages())
			arguments.push_back(" --no-color");
		arguments.push_back(dft.getFileRealPath());
        std::vector<File> outputs;
		outputs.push_back(exp);
		outputs.push_back(svl);
		if (exec.runCommand(dft2lntcExec.getFilePath(), arguments, "dft2lntc", outputs) == "")
			return 1;
	} else {
		messageFormatter->reportAction("Reusing DFT to EXP translation result",VERBOSITY_FLOW);
	}

	if (expOnly)
		return 0;

	std::string* tmpContents = FileSystem::load(exp);
	if (cachedResults.find(*tmpContents) != cachedResults.end()) {
		ret = cachedResults[*tmpContents];
		delete tmpContents;
		return 0;
	}
	std::string expContents = *tmpContents;
	delete tmpContents;

	if (useConverter == DFT::converter::SVL) {
#ifndef HAVE_CADP
		messageFormatter->reportError("CADP support has not been compiled in, but is required by your requested analysis.");
		return 1;
#else
		if (!reuse || !FileSystem::exists(bcg)) {
			// svl, exp -> bcg
			messageFormatter->reportAction("Building IMC...",VERBOSITY_FLOW);
			std::vector<std::string> arguments;
			arguments.push_back(svl.getFileRealPath());
			if (exec.runCommand(svlExec.getFilePath(), arguments, "svl", bcg) == "")
				return 1;

			messageFormatter->reportAction("Applying maximal progress to IMC...",VERBOSITY_FLOW);
			arguments = std::vector<std::string>();
			arguments.push_back(bcg.getFileRealPath());
			arguments.push_back(aut.getFileRealPath());
			arguments.push_back("FAIL");
			arguments.push_back("ONLINE");

			if (exec.runCommand(maxprogExec.getFilePath(), arguments, "maxprog", aut) == "")
				return 1;

			arguments = std::vector<std::string>();
			arguments.push_back(aut.getFileRealPath());
			arguments.push_back(bcg.getFileRealPath());

			if (exec.runCommand(bcgioExec.getFilePath(), arguments, "bcg_io", bcg) == "")
				return 1;

			arguments = std::vector<std::string>();
			arguments.push_back("-branching");
			arguments.push_back("-rate");
			arguments.push_back("-self");
			arguments.push_back("-epsilon");
			arguments.push_back("5e-324");
			arguments.push_back(bcg.getFileRealPath());

			if (exec.runCommand(bcgminExec.getFilePath(), arguments, "bcg_min", bcg) == "")
				return 1;
		} else {
			messageFormatter->reportAction("Reusing IMC",VERBOSITY_FLOW);
		}

		// obtain memtime result from svl
		if(Shell::readMemtimeStatisticsFromLog(svlLog,stats)) {
			messageFormatter->reportWarning("Could not read from svl log file `" + svlLog.getFileRealPath() + "'");
		}

		// test for non-determinism
		messageFormatter->reportAction("Testing for non-determinism...",VERBOSITY_FLOW);
		std::vector<std::string> arguments;
		arguments.push_back("-hidden");
		arguments.push_back(bcg.getFileRealPath());
		std::string hids = exec.runCommand(
				bcginfoExec.getFilePath(),
				arguments,
				"bcg_info");
		if (hids == "")
			return 1;
		if (hasHiddenLabels(File(hids))) {
			if (warnNonDeterminism) {
				messageFormatter->reportWarning("Non-determinism detected... you will want to ask for both 'min' and 'max' analysis results!");
			} else {
				messageFormatter->notify("Non-determinism detected... you will want to ask for both 'min' and 'max' analysis results!");
			}
		} else {
			messageFormatter->notify("No non-determinism detected.");
		}

		// test for composition errors.
		messageFormatter->reportAction("Testing for composition/modelling errors...",VERBOSITY_FLOW);
		arguments = std::vector<std::string>();
		arguments.push_back("-labels");
		arguments.push_back(bcg.getFileRealPath());

		std::string labs = exec.runCommand(
				bcginfoExec.getFilePath(),
				arguments,
				"bcg_info");
		if (labs == "")
			return 1;

		if (hasImpossibleLabel(File(labs))) {
			messageFormatter->reportError("Error composing model: 'IMPOSSIBLE' transitions reachable!");
			return 1;
		} else {
			messageFormatter->notify("No impossible labels detected.");
		}
#endif /* HAVE_CADP */
	} else if (!reuse
	           || (useChecker == IMRMC && !FileSystem::exists(exactTra))
	           || (useChecker == STORM && !FileSystem::exists(jani))
	           || (useChecker == MODEST && !FileSystem::exists(jani)))
	{
		/* DFTRES Converter to tra/lab */
		std::vector<File> outputs;
		std::vector<std::string> arguments;
		arguments.push_back("-jar");
		arguments.push_back(dftresJar.getFileRealPath());

		if (useChecker == IMRMC) {
			messageFormatter->reportAction("Building CTMC...",VERBOSITY_FLOW);
			arguments.push_back("--export-tralab");
			arguments.push_back(tra.newWithExtension("exact").getFileRealPath());
			outputs.push_back(exactTra);
			outputs.push_back(exactLab);
		} else {
			messageFormatter->reportAction("Building JANI...",VERBOSITY_FLOW);
			arguments.push_back("--export-jani");
			arguments.push_back(jani.getFileRealPath());
			outputs.push_back(jani);
		}
		arguments.push_back(exp.getFileRealPath());

		if (exec.runCommand(javaExec.getFilePath(), arguments, "dftres", outputs) == "")
			return 1;
	}

	std::unique_ptr<Checker> checker;

	switch (useChecker) {
	case MRMC:
#ifndef HAVE_CADP
		messageFormatter->reportError("CADP is not compiled in, but is required for MRMC analysis.");
		return 1;
#else
		if(!reuse || !FileSystem::exists(ctmdpi)) {
			// bcg -> ctmdpi, lab
			messageFormatter->reportAction("Translating IMC to CTMDPI...",VERBOSITY_FLOW);
			std::vector<std::string> arguments;
			arguments.push_back("-a");
			arguments.push_back("FAIL");
			arguments.push_back("-o");
			arguments.push_back(ctmdpi.getFileRealPath());
			arguments.push_back(bcg.getFileRealPath());

			if (exec.runCommand(imc2ctmdpExec.getFilePath(), arguments, "imc2ctmdpi", ctmdpi) == "")
				return 1;
		} else {
			messageFormatter->reportAction("Reusing IMC to CTMDPI translation result",VERBOSITY_FLOW);
		}

		checker = std::unique_ptr<Checker>(new MRMCRunner(messageFormatter, &exec, false, mrmcExec, ctmdpi, lab));
		break;
#endif /* HAVE_CADP */
	case IMRMC:
		if(useConverter != DFTRES && (!reuse || !FileSystem::exists(tra))) {
#ifndef HAVE_CADP
			messageFormatter->reportError("Internal error: tried to use bcg2jani in non-CADP program.");
			return 1;
#else
			// bcg -> tra, lab
			messageFormatter->reportAction("Translating IMC to .tra/.lab ...",VERBOSITY_FLOW);
			std::vector<std::string> arguments;
			arguments.push_back(bcg.getFileRealPath());
			arguments.push_back(tra.newWithExtension("").getFileRealPath());
			arguments.push_back("FAIL");
			arguments.push_back("ONLINE");

			if (exec.runCommand(bcg2tralabExec.getFilePath(), arguments, "bcg2tralab", tra) == "")
				return 1;
#endif /* HAVE_CADP */
		} else {
			messageFormatter->reportAction("Reusing IMC to .tra/.lab translation result",VERBOSITY_FLOW);
		}
		{
			File tmpTra, tmpLab;
			if (useConverter == SVL) {
				tmpTra = tra;
				tmpLab = lab;
			} else {
				tmpTra = exactTra;
				tmpLab = exactLab;
			}
			checker = std::unique_ptr<Checker>(new MRMCRunner(messageFormatter, &exec, true, imrmcExec, tmpTra, tmpLab));
		}
		break;
	case IMCA:
#ifndef HAVE_CADP
		messageFormatter->reportError("CADP is not compiled in, but is required for IMCA analysis.");
		return 1;
#else
		if(!reuse || !FileSystem::exists(ma)) {
			// bcg -> ma
			messageFormatter->reportAction("Translating IMC to IMCA format...",VERBOSITY_FLOW);
			std::vector<std::string> arguments;
			arguments.push_back(bcg.getFileRealPath());
			arguments.push_back(ma.getFileRealPath());
			arguments.push_back("FAIL");

			if (exec.runCommand(bcg2imcaExec.getFilePath(), arguments, "bcg2imca", ma) == "")
				return 1;
		} else {
			messageFormatter->reportAction("Reusing IMC to IMCA format translation result",VERBOSITY_FLOW);
		}
		checker = std::unique_ptr<Checker>(new IMCARunner(messageFormatter, &exec, imcaExec, ma));

		break;
#endif /* HAVE_CADP */
	case STORM: {
		if(useConverter == SVL && (!reuse || !FileSystem::exists(jani))) {
#ifndef HAVE_CADP
			messageFormatter->reportError("Internal error: tried to use bcg2jani in non-CADP program.");
			return 1;
#else
			// bcg -> jani
			messageFormatter->reportAction("Translating IMC to JANI format...",VERBOSITY_FLOW);
			std::vector<std::string> arguments;
			arguments.push_back(bcg.getFileRealPath());
			arguments.push_back(jani.getFileRealPath());
			arguments.push_back("FAIL");
			arguments.push_back("ONLINE");

			if (exec.runCommand(bcg2janiExec.getFilePath(), arguments, "bcg2jani", jani) == "")
				return 1;
#endif /* HAVE_CADP */
		} else if (useConverter == SVL) {
			messageFormatter->reportAction("Reusing IMC to JANI format translation result",VERBOSITY_FLOW);
		}
		
		StormRunner *sr = new StormRunner(messageFormatter, &exec, stormExec, jani, exactMode, useConverter == DFTRES);
		checker = std::unique_ptr<Checker>(sr);
		break;
	}
	case MODEST:
		if(useConverter == SVL && (!reuse || !FileSystem::exists(jani))) {
#ifndef HAVE_CADP
			messageFormatter->reportError("Internal error: tried to use bcg2jani in non-CADP program.");
			return 1;
#else
			// bcg -> jani
			messageFormatter->reportAction("Translating IMC to JANI format...",VERBOSITY_FLOW);
			std::vector<std::string> arguments;
			arguments.push_back(bcg.getFileRealPath());
			arguments.push_back(jani.getFileRealPath());
			arguments.push_back("FAIL");
			arguments.push_back("ONLINE");

			if (exec.runCommand(bcg2janiExec.getFilePath(), arguments, "bcg2jani", jani) == "")

				return 1;
#endif /* HAVE_CADP */
		} else if (useConverter == SVL) {
			messageFormatter->reportAction("Reusing IMC to JANI format translation result",VERBOSITY_FLOW);
		}
		
		{
			ModestRunner *sr = new ModestRunner(messageFormatter, &exec, "mcsta", jani);
			checker = std::unique_ptr<Checker>(sr);
		}
		break;
	default:
		messageFormatter->reportError("Unexpected checker type");
	}

	std::vector<DFT::DFTCalculationResultItem> results;
	results = checker->analyze(queries);
	ret.failProbs.insert(ret.failProbs.end(), results.begin(), results.end());
	ret.stats = stats;

	if(!buildDot.empty()) {
#ifndef HAVE_CADP
		messageFormatter->reportAction("DOT output required CADP, but CADP support is not compiled in.");
		return 1;
#else
		// bcg -> dot
		messageFormatter->reportAction("Building DOT from IMC...",VERBOSITY_FLOW);
		std::vector<std::string> arguments;
		arguments.push_back(bcg.getFileRealPath());
		arguments.push_back(dot.getFileRealPath());
		
		if (exec.runCommand(bcgioExec.getFilePath(), arguments, "bcg_io", dot) == "")
			return 1;
		
		// dot -> png
		messageFormatter->reportAction("Translating DOT to " + buildDot + "...",VERBOSITY_FLOW);
		arguments.clear();
		arguments.push_back("-T");
		arguments.push_back(buildDot);
		arguments.push_back(dot.getFileRealPath());
		arguments.push_back("-o");
		arguments.push_back(png.getFileRealPath());
		if (exec.runCommand(dotExec.getFilePath(), arguments, "dot", png) == "")
			return 1;
#endif /* HAVE_CADP */
	}

	cachedResults[expContents] = ret;
	return 0;
}

int main(int argc, char** argv) {
	/* Command line arguments and their default settings */
	string timeSpec           = "1";
	int    timeSpecSet        = 0;
	string timeIntervalLwb    = "";
	string timeIntervalUpb    = "";
	string timeIntervalStep   = "";
	int    timeIntervalSet    = 0;
	string timeLwb            = "";
	string timeUpb            = "";
	int    timeLwbUpbSet      = 0;
	string yamlFileName       = "";
	int    yamlFileSet        = 0;
	string csvFileName        = "";
	int    csvFileSet         = 0;
	string dotToType          = "png";
	int    dotToTypeSet       = 0;
	string outputFolder       = "output";
	string calcCommand        = "";
	int    calcCommandSet     = 0;
	bool   steadyState        = 0;
	int    mttf               = 0;
	string errorBound         = "";
	int    errorBoundSet      = 0;

	int verbosity            = 0;
	bool warnNonDeterminism  = true;
	bool modularize          = false;
	int print                = 0;
	int reuse                = 0;
	int useColoredMessages   = 1;
	int printHelp            = 0;
	string printHelpTopic    = "";
	int printVersion         = 0;
	enum DFT::checker useChecker = DFT::checker::STORM;
	enum DFT::converter useConverter = DFT::converter::SVL;
	bool explicitChecker     = false;
	bool checkMin            = false;
	bool minMaxSet           = false;
	bool expOnly             = false;
	bool exactMode           = false;
	
	std::vector<std::string> failedBEs;
	std::vector<Query> queries;

	/* Create a new compiler context */
	MessageFormatter* messageFormatter = new MessageFormatter(std::cerr);

	/* Parse command line arguments */
	int argi;
	for (argi = 1; argi < argc; argi++) {
		if (argv[argi][0] != '-')
			break;
		if (argv[argi][1] == '-' && argv[argi][2] == 0) {
			argi++;
			break;
		}

		if (!strcmp(argv[argi], "-C")) {
			// -C FILE
			outputFolder = string(argv[++argi]);
		} else if (!strcmp(argv[argi], "-r")) {
			// -r FILE
			yamlFileName = string(argv[++argi]);
			yamlFileSet = 1;
		} else if (!strcmp(argv[argi], "-c")) {
			// -c FILE
			csvFileName = string(argv[++argi]);
			csvFileSet = 1;
		} else if (!strcmp(argv[argi], "-p")) {
			print = 1;
		} else if (!strcmp(argv[argi], "-x")) {
			useChecker = DFT::checker::EXP_ONLY;
			expOnly = true;
		} else if (!strcmp(argv[argi], "-R")) {
			reuse = 1;
		} else if (!strcmp(argv[argi], "-E")) {
			// -E Error bound
			errorBound = string(argv[++argi]);
			errorBoundSet = true;
		} else if (!strcmp(argv[argi], "-f")) {
			// -f MRMC/IMCA Command
			calcCommand = string(argv[++argi]);
			calcCommandSet = true;
		} else if (!strcmp(argv[argi], "-m")) {
			mttf = 1;
		} else if (!strcmp(argv[argi], "-M")) {
			modularize = 1;
		} else if (!strcmp(argv[argi], "-s")) {
			steadyState = 1;
		} else if (!strcmp(argv[argi], "-t")) {
			// -t STRING containing time values separated by whitespace
			timeSpec = string(argv[++argi]);
			timeSpecSet = 1;
		} else if (!strcmp(argv[argi], "-u")) {
			timeSpec = string("Eventually");
			timeSpecSet = 1;
		} else if (!strcmp(argv[argi], "-i")) {
			// -i STRING STRING STRING
			timeIntervalLwb = string(argv[++argi]);
			timeIntervalUpb = string(argv[++argi]);
			timeIntervalStep = string(argv[++argi]);
			timeIntervalSet = 1;
		} else if (!strcmp(argv[argi], "-I")) {
			// -I STRING STRING
			timeLwb = string(argv[++argi]);
			timeUpb = string(argv[++argi]);
			timeLwbUpbSet = 1;
		} else if (!strcmp(argv[argi], "-h")) {
			printHelp = true;
			break;
		} else if (!strncmp(argv[argi], "-v", 2)) {
			for (int i = 1; i < strlen(argv[argi]); i++) {
				if (argv[argi][i] == 'v') {
					++verbosity;
				} else {
					messageFormatter->reportError(std::string("Unknown argument: ") + argv[argi]);
					printHelp = true;
					break;
				}
			}
		} else if (!strcmp(argv[argi], "-q")) {
			verbosity = 0;
		} else if (!strcmp(argv[argi], "-e")) {
			// -e EVIDENCE
			const char* begin = argv[++argi];
			const char* end = begin;
			while(*begin) {
				end = begin;
				while(*end && *end != ',')
					++end;
				if(begin < end) {
					failedBEs.push_back(std::string(begin, end));
				}
				if(!*end)
					break;
				begin = end + 1;
			}
		} else if(!strncmp("--help", argv[argi], 6)) {
			printHelp = true;
			if(strlen(argv[argi]) > 7 && argv[argi][6]=='=') {
				printHelpTopic = string(argv[argi] + 7);
			}
		} else if(!strcmp("--version", argv[argi])) {
			printVersion = true;
		} else if(!strcmp("--color", argv[argi])) {
			useColoredMessages = true;
		} else if(!strncmp("--dot", argv[argi],5)) {
			dotToTypeSet = true;
			if(strlen(argv[argi]) > 6 && argv[argi][5]=='=') {
				dotToType = string(argv[argi] + 6);
				cerr << "DOT: " << dotToType << endl;
			}
		} else if(!strncmp("--verbose", argv[argi], 9)) {
			if(strlen(argv[argi]) > 10 && argv[argi][9] == '=') {
				verbosity = atoi(argv[argi] + 10);
			} else if(strlen(argv[argi]) == 9) {
				++verbosity;
			}
		} else if(!strcmp("--no-color", argv[argi])) {
			useColoredMessages = false;
		} else if(!strcmp("--no-nd-warning", argv[argi])) {
			warnNonDeterminism = false;
		} else if(!strcmp("--min", argv[argi])) {
			checkMin = true;
			minMaxSet = true;
		} else if(!strcmp("--max", argv[argi])) {
			checkMin = false;
			minMaxSet = true;
		}
		if (!strcmp("--mrmc", argv[argi])) {
			useChecker = DFT::checker::MRMC;
			explicitChecker = true;
		} else if (!strcmp("--imrmc", argv[argi])) {
			useChecker = DFT::checker::IMRMC;
			explicitChecker = true;
		} else if (!strcmp("--imca", argv[argi])) {
			useChecker = DFT::checker::IMCA;
			explicitChecker = true;
		} else if (!strcmp("--storm", argv[argi])) {
			useChecker = DFT::checker::STORM;
			explicitChecker = true;
		} else if (!strcmp("--modest", argv[argi])) {
			useChecker = DFT::checker::MODEST;
			explicitChecker = true;
		} else if (!strcmp("--exact", argv[argi])) {
			exactMode = true;
			useConverter = DFT::converter::DFTRES;
			if (!explicitChecker)
				useChecker = DFT::checker::IMRMC;
		}
	}
	if (expOnly)
		useChecker = DFT::checker::EXP_ONLY;

	messageFormatter->useColoredMessages(useColoredMessages);
	messageFormatter->setVerbosity(verbosity);
	messageFormatter->setAutoFlush(true);

#ifndef HAVE_CADP
	if (useConverter == DFT::converter::SVL) {
		messageFormatter->reportWarningAt(Location("commandline"),"CADP mode not compiled in, using DFTRES for conversion");
		useConverter = DFT::converter::DFTRES;
	}
#endif

	if (exactMode
		&& useChecker != DFT::checker::EXP_ONLY
		&& useChecker != DFT::checker::IMRMC
		&& useChecker != DFT::checker::MODEST
		&& useChecker != DFT::checker::STORM)
	{
		messageFormatter->reportWarningAt(Location("commandline"),"Exact flag has can only be used with IMRMC, Storm, or Modest, defaulting to IMRMC");
		useChecker = DFT::checker::IMRMC;
	}

	Query q;
	if (errorBoundSet) {
		double t;
		if (!isReal(errorBound, &t) || t<=0) {
			messageFormatter->reportErrorAt(Location("commandline -E flag"),"Given error bound is not a positive real: "+errorBound);
		}
		q.errorBound = decnumber<>(errorBound);
	}
	q.errorBoundSet = errorBoundSet;

	if (minMaxSet)
		q.min = checkMin;
	else if (useChecker != DFT::checker::MODEST)
		q.min = false;

	if (mttf && modularize) {
		messageFormatter->reportWarningAt(Location("commandline"),"MTTF flag (-m) has been given: Disabling modularization.");
		modularize = false;
	}
	if (mttf && useChecker == DFT::checker::MRMC) {
		messageFormatter->reportWarningAt(Location("commandline"), "MTTF flag cannot be used with MRMC model checker, defaulting to Storm.");
		useChecker = DFT::checker::STORM;
	}
	if (steadyState && useChecker == DFT::checker::MRMC) {
		messageFormatter->reportWarningAt(Location("commandline"), "Steady-state flag cannot currently be used with MRMC model checker, defaulting to Storm.");
		useChecker = DFT::checker::STORM;
	}
	if (steadyState && useChecker == DFT::checker::IMCA) {
		messageFormatter->reportWarningAt(Location("commandline"), "Steady-state flag cannot currently be used with IMCA model checker, defaulting to Storm.");
		useChecker = DFT::checker::STORM;
	}
	if (steadyState && useChecker == DFT::checker::MODEST) {
		messageFormatter->reportWarningAt(Location("commandline"), "Steady-state flag cannot currently be used with Modest model checker, defaulting to Storm.");
		useChecker = DFT::checker::STORM;
	}
	if (mttf) {
		Query mq = q;
		mq.type = EXPECTEDTIME;
		queries.push_back(mq);
	}
	if (steadyState) {
		Query sq = q;
		sq.type = STEADY;
		queries.push_back(sq);
	}

	if (timeLwbUpbSet) {
		double tl;
		if(!isReal(timeLwb, &tl) || tl<0) {
			messageFormatter->reportErrorAt(Location("commandline -I flag"),"Given interval lwb is not a non-negative real: "+timeLwb);
		}
		double tu;
		if(!isReal(timeUpb, &tu) || tu<=0) {
			messageFormatter->reportErrorAt(Location("commandline -I flag"),"Given interval upb is not a positive real: "+timeUpb);
		}
		Query tq = q;
		tq.type = TIMEBOUND;
		tq.lowerBound = timeLwb;
		tq.upperBound = timeUpb;
		tq.step = (intmax_t)-1;
		queries.push_back(tq);
	}

	if (!calcCommandSet && !timeIntervalSet && !timeLwbUpbSet && !steadyState && !mttf)
		timeSpecSet = 1; // default

	/* Print help / version if requested and quit */
	if(printHelp) {
		print_help(messageFormatter,printHelpTopic);
		exit(0);
	}
	if(printVersion) {
		print_version(messageFormatter);
		exit(0);
	}
	
	if(calcCommandSet) {
		Query cq = q;
		cq.type = CUSTOM;
		cq.customQuery = calcCommand;
		queries.push_back(cq);
	}
	if (timeSpecSet && timeSpec == string("Eventually")) {
		Query uq = q;
		uq.type = UNBOUNDED;
		queries.push_back(uq);
	} else if (timeSpecSet) {
		std::string str = timeSpec;
		size_t b, e;
		bool hasValidItems = false;
		bool hasInvalidItems = false;
		std::string sep(" \t,;"); // separator characters that we allow
		while((b=str.find_first_not_of(sep)) != string::npos) {
			//cerr << "str: \"" + str + "\"" << endl;
			//cerr << "b: " << b << endl;
			// find first separator character
			e=str.substr(b).find_first_of(sep);
			//cerr << "e: " << e << endl;
			std::string s;
			s = str.substr(b,e);
			if (e!=string::npos) {
				str = str.substr(b+e+1);
			} else {
				str = "";
			}
			//cerr<< "s: \"" << s << "\"" << endl;
			double t;
			if(!isReal(s, &t) || t<0) {
				hasInvalidItems = true;
				messageFormatter->reportErrorAt(Location("commandline -t flag"),"Given mission time value is not a non-negative real: "+s);
			} else {
				Query tq = q;
				tq.type = TIMEBOUND;
				tq.lowerBound = (uintmax_t)0;
				tq.upperBound = s;
				tq.step = (intmax_t)-1;
				queries.push_back(tq);
				hasValidItems = true;
			}
		}
		if (!hasInvalidItems && !hasValidItems) {
			messageFormatter->reportErrorAt(Location("commandline -t flag"),"Given mission time list of values is empty");
		}
	} else if (timeIntervalSet) {
		double lwb;
		if(!isReal(timeIntervalLwb, &lwb) || lwb<0) {
			messageFormatter->reportErrorAt(Location("commandline -i flag"),"Given interval lwb is not a non-negative real: "+timeIntervalLwb);
		}
		double upb;
		if(!isReal(timeIntervalUpb, &upb) || upb<=0) {
			messageFormatter->reportErrorAt(Location("commandline -i flag"),"Given interval upb is not a positive real: "+timeIntervalUpb);
		}
		double step;
		if(!isReal(timeIntervalStep, &step) || step<=0) {
			messageFormatter->reportErrorAt(Location("commandline -i flag"),"Given interval step is not a positive real: "+timeIntervalStep);
		}
		if (lwb > upb) {
			messageFormatter->reportErrorAt(Location("commandline -i flag"),"Given interval is empty (lwb > upb)");
		}
		/* Check if all went OK so far */
		if(messageFormatter->getErrors()>0) {
			return -1;
		}
		Query iq = q;
		iq.type = TIMEBOUND;
		iq.lowerBound = timeIntervalLwb;
		iq.upperBound = timeIntervalUpb;
		iq.step = timeIntervalStep;
		queries.push_back(iq);
	}
	
	/* Parse command line arguments without a -X.
	 * These specify the input files.
	 */
	vector<File> dfts;
	for (; argi < argc; argi++) {
		if(!strcmp(argv[argi], "-")) {
			dfts.push_back(string());
		} else {
			dfts.push_back(File(string(argv[argi])).fix());
		}
	}
	
	/* Enable Shell messages */
	Shell::messageFormatter = messageFormatter;
	
	/* Create the DFTCalc class */
	DFT::DFTCalc calc(messageFormatter);
	if(dotToTypeSet) calc.setBuildDOT(dotToType);
	
	/* Check if all needed tools are available */
	if(calc.checkNeededTools(useChecker, useConverter)) {
		messageFormatter->reportError("There was an error with the environment");
		return -1;
	}

	calc.setEvidence(failedBEs);

	/* Check if all went OK so far */
	if(messageFormatter->getErrors()>0) {
		return -1;
	}
	
	/* Change the CWD to ./output, creating the folder if not existent */
	File outputFolderFile = File(outputFolder).fix();
	FileSystem::mkdir(outputFolderFile);
	PushD workdir(outputFolderFile);
			
	/// A map containing the results of the calculation. <filename> --> <result>
	map<std::string,DFT::DFTCalculationResult> results;
		
	/* Calculate DFTs */
	bool hasInput = false;
	bool hasErrors = false;
	for(File dft: dfts) {
		hasInput = true;
		if(FileSystem::exists(dft)) {
			DFT::DFTCalculationResult ret;
			bool res;
			try {
				if (!modularize) {
					res = calc.calculateDFT(reuse, outputFolderFile.getFileRealPath(),dft, queries, useChecker, useConverter, warnNonDeterminism, "", ret, expOnly, exactMode);
				} else {
					res = calc.calcModular(reuse, outputFolderFile.getFileRealPath(),dft, queries, useChecker, useConverter, warnNonDeterminism, ret, expOnly, exactMode);
				}
			} catch (std::exception &e) {
				messageFormatter->reportError(e.what());
				res = true;
			}
			results[dft.getFileName()] = ret;
			hasErrors = hasErrors || res;
		} else {
			messageFormatter->reportError("DFT File `" + dft.getFileRealPath() + "' does not exist");
		}
	}
	workdir.popd();
	if (expOnly)
		return 0;

	if (hasErrors) {
		return(1);
	}
	
	/* If there were no DFT calculations performed... */
	if(!hasInput) {
		messageFormatter->reportWarning("No calculations performed");
	}
	
	/* Show results */
	if(verbosity>0 || print) {
		if(calcCommandSet) {
			messageFormatter->notify("Using: " + calcCommand);
		} else if (timeIntervalSet) {
			messageFormatter->notify("Within time interval: [" + timeIntervalLwb + " .. " + timeIntervalUpb +"], stepsize " + timeIntervalStep);
		} else {
			messageFormatter->notify("Within time units: " + timeSpec);
		}
		std::stringstream out;
		for(auto it: results) {
			std::string fName = it.first;
			for(auto it2: it.second.failProbs) {
				if (it2.query.type == EXPECTEDTIME) {
					out << "MTTF(`" << fName << "'" << ")=" << it2.valStr() << std::endl;
				} else {
					out << "P(`" << fName << "'" << ", " << it2.mrmcCommand << ", " << it2.missionTime << ", " << "fails)=" << it2.valStr() << std::endl;;
				}
				if (!it2.exactBounds) {
					double b = 0.5*(double)(it2.upperBound + it2.lowerBound);
					double eb = (double)it2.query.errorBound;
					if (eb > b / 10) {
						out << "WARNING: Value is too small for requested error bound (" << it2.query.errorBound.str() << "), best guess: [" << it2.lowerBound.str() << "; " << it2.upperBound.str() << "]\n";
					}
				}
			}
		}
		std::cout << out.str();
	}
    
	if (useConverter != DFT::converter::DFTRES) {
		for(File dft: dfts) {
			File svlLogFile = File(outputFolderFile.getFileRealPath(),dft.getFileBase(),"log");
			Shell::SvlStatistics svlStats;
			if(Shell::readSvlStatisticsFromLog(svlLogFile,svlStats)) {
				messageFormatter->reportWarning("Could not read from svl log file `" + svlLogFile.getFileRealPath() + "'");
			} else {
				messageFormatter->reportAction2("Read from svl log file `" + svlLogFile.getFileRealPath() + "'",VERBOSITY_DATA);
				messageFormatter->notify("SVL composition maximum values");
				std::cout << "Max States: " << svlStats.max_states << ", Max Transitions: " << svlStats.max_transitions << ", Max Memory by SVL: " << svlStats.max_memory << " Kbytes" << endl;
			}
		}
	}
	
	/* Write yaml file */
	if(yamlFileSet) {
		YAML::Emitter out;
		out << results;
		if(yamlFileName=="") {
			std::cout << string(out.c_str()) << std::endl;
		} else {
			std::ofstream yamlFile(yamlFileName);
			if(yamlFile.is_open()) {
				messageFormatter->notify("Printing yaml to file: " + yamlFileName);
				yamlFile << string(out.c_str()) << std::endl;
				yamlFile.flush();
			} else {
				messageFormatter->reportErrorAt(Location(yamlFileName),"could not open file for printing yaml");
			}
		}
	}
	
	/* Write csv file */
	if(csvFileSet) {
		std::stringstream out;
		if (mttf) {
			out << "Mean Time to Failure" << std::endl;
			for(auto it: results) {
				std::string fName = it.first;
				for(auto it2: it.second.failProbs) {
					out << it2.valStr() << std::endl;
				}
			}
		} else {
			out << "Time" << ", " << "Unreliability" << std::endl;
			for(auto it: results) {
				std::string fName = it.first;
				for(auto it2: it.second.failProbs) {
					out << it2.missionTime << ", " << it2.valStr() << std::endl;
				}
			}
		}
		if(csvFileName=="") {
			std::cout << out.str();
		} else {
			std::ofstream csvFile(csvFileName);
			if(csvFile.is_open()) {
				messageFormatter->notify("Printing csv to file: " + csvFileName);
				csvFile << out.str();
				csvFile.flush();
			} else {
				messageFormatter->reportErrorAt(Location(csvFileName),"could not open file for printing csv");
			}
		}
	}
	
	/* Free the messager */
	delete messageFormatter;
	
	return 0;
}

bool DFT::DFTCalc::findInPath(std::string tool, File &ret) {
	bool accessible = false;
	vector<File> progs;
	FileSystem::findInPath(progs, File(tool));
	for(File bin: progs) {
		accessible = false;
		if(FileSystem::hasAccessTo(bin, X_OK)) {
			accessible = true;
			ret = bin;
			break;
		} else {
			messageFormatter->reportWarning(tool + " [" + bin.getFilePath() + "] is not runnable", VERBOSITY_SEARCHING);
		}
	}
	if(!accessible) {
		messageFormatter->reportError("no runnable " + tool + " executable found in PATH");
	} else {
		messageFormatter->reportAction("Using " + tool + " [" + ret.getFilePath() + "]",VERBOSITY_SEARCHING);
	}
	return accessible;
}

bool DFT::DFTCalc::checkNeededTools(DFT::checker checker, converter conv) {
	bool ok = true;

	messageFormatter->notify("Checking environment...",VERBOSITY_SEARCHING);

	/* Obtain all needed root information from environment */
	dft2lntRoot = getRoot();
#ifdef HAVE_CADP
	if (checker == MRMC) {
		coralRoot = getCoralRoot();
		imc2ctmdpExec = File(coralRoot+"/bin/imc2ctmdp");
	} else if (checker == IMCA) {
		imcaRoot = getImcaRoot();
		bcg2imcaExec = File(imcaRoot+"/bin/bcg2imca");
	} else if (checker == STORM || checker == MODEST) {
		bcg2janiExec = File(dft2lntRoot+"/bin/bcg2jani");
	} else if (checker == IMRMC) {
		bcg2tralabExec = File(dft2lntRoot+"/bin/bcg2tralab");
		maxprogExec = File(dft2lntRoot+"/bin/maxprog");
	}
	maxprogExec = File(dft2lntRoot+"/bin/maxprog");

	cadpRoot = getCADPRoot();
	if (cadpRoot != "")
		svlExec = File(cadpRoot+"/com/svl");
#endif

	/* These tools should be easy to find in the roots. Note that an added
	 * bonus would be to locate them using PATH as well.
	 */
	dft2lntcExec = File(dft2lntRoot+"/bin/dft2lntc");
	if (!FileSystem::exists(dft2lntcExec)) {
		if (FileSystem::exists(File(dft2lntRoot + "/bin/dft2lntc.exe")))
			dft2lntcExec = File(dft2lntRoot + "/bin/dft2lntc.exe");
	}

	/* Find dft2lntc executable (based on DFT2LNTROOT environment variable) */
	if(dft2lntRoot.empty()) {
		messageFormatter->reportError("Environment variable `DFT2LNTROOT' not set. Please set it to where /bin/dft2lntc can be found.");
		ok = false;
	} else if(!FileSystem::isDir(File(dft2lntRoot))) {
		messageFormatter->reportError("Could not enter dft2lntroot directory (environment variable `DFT2LNTROOT'");
		ok = false;
	} else if(!FileSystem::exists(dft2lntcExec)) {
		messageFormatter->reportError("dft2lntc not found (in " + dft2lntRoot + "/bin)");
		ok = false;
	} else if(!FileSystem::hasAccessTo(dft2lntcExec,X_OK)) {
		messageFormatter->reportError("dft2lntc not executable (in " + dft2lntRoot+"/bin)");
		ok = false;
	} else {
		messageFormatter->reportAction("Using dft2lntc [" + dft2lntcExec.getFilePath() + "]",VERBOSITY_SEARCHING);
	}

	/* Find imc2ctmdpi executable (based on CORAL environment variable) */
	if(checker == MRMC && coralRoot.empty()) {
		messageFormatter->reportError("Environment variable `CORAL' not set. Please set it to where coral can be found.");
		ok = false;
	} else if(checker == MRMC && !FileSystem::hasAccessTo(File(coralRoot),X_OK)) {
		messageFormatter->reportError("Could not enter coral directory (environment variable `CORAL'");
		ok = false;
#ifdef HAVE_CADP
	} else if(checker == MRMC && !FileSystem::hasAccessTo(imc2ctmdpExec,F_OK)) {
		messageFormatter->reportError("imc2ctmdp not found (in " + coralRoot+"/bin)");
		ok = false;
	} else if(checker == MRMC && !FileSystem::hasAccessTo(imc2ctmdpExec,X_OK)) {
		messageFormatter->reportError("imc2ctmdp not executable (in " + coralRoot+"/bin)");
		ok = false;
	} else if (checker == MRMC) {
		messageFormatter->reportAction("Using imc2ctmdp [" + imc2ctmdpExec.getFilePath() + "]",VERBOSITY_SEARCHING);
#endif
	}


#ifdef HAVE_CADP
	/* Find bcg2imca */
	if(checker == IMCA && !FileSystem::hasAccessTo(bcg2imcaExec,F_OK)) {
		messageFormatter->reportError("bcg2imca not found (in " + imcaRoot+"/bin)");
		ok = false;
	} else if(checker == IMCA && !FileSystem::hasAccessTo(bcg2imcaExec,X_OK)) {
		messageFormatter->reportError("bcg2imca not executable (in " + imcaRoot+"/bin)");
		ok = false;
	} else if (checker == IMCA) {
		messageFormatter->reportAction("Using bcg2imca [" + bcg2imcaExec.getFilePath() + "]",VERBOSITY_SEARCHING);
	}

	if (conv == SVL) {
		/* Find svl executable (based on CADP environment variable) */
		if(cadpRoot.empty()) {
			messageFormatter->reportError("Environment variable `CADP' not set. Please set it to where CADP can be found.");
			ok = false;
		} else if(!FileSystem::hasAccessTo(File(cadpRoot),X_OK)) {
			messageFormatter->reportError("Could not enter CADP directory (environment variable `CADP'");
			ok = false;
		} else {
			if(!FileSystem::hasAccessTo(svlExec,F_OK)) {
				messageFormatter->reportError("svl not found (in " + cadpRoot+"/com)");
				ok = false;
			} else if(!FileSystem::hasAccessTo(svlExec,X_OK)) {
				messageFormatter->reportError("svl not executable (in " + cadpRoot+"/com)");
				ok = false;
			} else {
				messageFormatter->reportAction("Using svl [" + svlExec.getFilePath() + "]",VERBOSITY_SEARCHING);
			}
		}
		ok &= findInPath("bcg_io", bcgioExec);
		ok &= findInPath("bcg_info", bcginfoExec);
		ok &= findInPath("bcg_min", bcgminExec);
	}
#endif

	if (conv == DFTRES) {
		dftresJar = getDftresJar();
		if(!FileSystem::exists(dftresJar))
			ok = false;
	}

#ifdef WIN32
	std::string executable_suffix = ".exe";
#else
	std::string executable_suffix = "";
#endif

	/* Find a java executable */
	if (conv == DFTRES) {
		dftresJar = getDftresJar();
		ok &= findInPath("java" + executable_suffix, javaExec);
	}

	/* Find a storm executable */
	if (checker == STORM)
		ok &= findInPath("storm" + executable_suffix, stormExec);

	/* Find an mrmc executable (based on PATH environment variable) */
	if (checker == MRMC)
		ok &= findInPath("mrmc" + executable_suffix, mrmcExec);

	/* Find an imrmc executable (based on PATH environment variable) */
	if (checker == IMRMC)
		ok &= findInPath("imrmc" + executable_suffix, imrmcExec);

	/* Find an imca executable (based on PATH environment variable) */
	if (checker == IMCA)
		ok &= findInPath("imca" + executable_suffix, imcaExec);

	/* Find dot executable (based on PATH environment variable) */
	if (!buildDot.empty())
		ok &= findInPath("dot" + executable_suffix, dotExec);

	return !ok;
}
