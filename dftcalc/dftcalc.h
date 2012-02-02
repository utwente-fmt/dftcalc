#ifndef DFTCALC_H
#define DFTCALC_H

#include <string>

class DFTCalc {
public:
	static const int VERBOSITY_SEARCHING;
	
private:
	std::string buildDot;
	MessageFormatter* messageFormatter;
	std::string dft2lntRoot;
	std::string coralRoot;
	std::string cadpRoot;
	File dft2lntcExec;
	File coralExec;
	File imc2ctmdpExec;
	File svlExec;
	File bcgioExec;
	File mrmcExec;
	File dotExec;

	std::string getCoralRoot(MessageFormatter* messageFormatter);
	std::string getRoot(MessageFormatter* messageFormatter);
	std::string getCADPRoot(MessageFormatter* messageFormatter);

	map<std::string,double> results;

public:
	
	const map<std::string,double> getResults() const {
		return results;
	}
	
	bool checkNeededTools() {
		
		bool ok = true;
		
		messageFormatter->notify("Checking tools...",VERBOSITY_SEARCHING);
		
		dft2lntRoot = getRoot(NULL);
		coralRoot = getCoralRoot(NULL);
		cadpRoot = getCADPRoot(NULL);
		
		dft2lntcExec = File(dft2lntRoot+"/bin/dft2lntc");
		imc2ctmdpExec = File(coralRoot+"/bin/imc2ctmdp");
		coralExec = File(coralRoot+"/coral");
		svlExec = File(cadpRoot+"/com/svl");
		
		// Find dft2lntc executable (based on DFT2LNTROOT environment variable)
		if(dft2lntRoot.empty()) {
			messageFormatter->reportError("Environment variable `DFT2LNTROOT' not set. Please set it to where /bin/dft2lntc can be found.");
			ok = false;
		} else if(!FileSystem::hasAccessTo(File(dft2lntRoot),X_OK)) {
			messageFormatter->reportError("Could not enter dft2lntroot directory (environment variable `DFT2LNTROOT'");
			ok = false;
		} else if(!FileSystem::hasAccessTo(dft2lntcExec,F_OK)) {
			messageFormatter->reportError("dft2lntc not found (in " + dft2lntRoot+"/bin)");
			ok = false;
		} else if(!FileSystem::hasAccessTo(dft2lntcExec,X_OK)) {
			messageFormatter->reportError("dft2lntc not executable (in " + dft2lntRoot+"/bin)");
			ok = false;
		} else {
			messageFormatter->reportAction("Using dft2lntc [" + dft2lntcExec.getFilePath() + "]",VERBOSITY_SEARCHING);
		}
		
		// Find imc2ctmdpi and coral executables (based on CORAL environment variable)
		if(coralRoot.empty()) {
			messageFormatter->reportError("Environment variable `CORAL' not set. Please set it to where coral can be found.");
			ok = false;
		} else if(!FileSystem::hasAccessTo(File(coralRoot),X_OK)) {
			messageFormatter->reportError("Could not enter coral directory (environment variable `CORAL'");
			ok = false;
		} else {
			if(!FileSystem::hasAccessTo(imc2ctmdpExec,F_OK)) {
				messageFormatter->reportError("imc2ctmdp not found (in " + coralRoot+"/bin)");
				ok = false;
			} else if(!FileSystem::hasAccessTo(imc2ctmdpExec,X_OK)) {
				messageFormatter->reportError("imc2ctmdp not executable (in " + coralRoot+"/bin)");
				ok = false;
			} else {
				messageFormatter->reportAction("Using imc2ctmdp [" + imc2ctmdpExec.getFilePath() + "]",VERBOSITY_SEARCHING);
			}
			if(!FileSystem::hasAccessTo(coralExec,F_OK)) {
				messageFormatter->reportError("coral not found (in " + coralRoot+"/bin)");
				ok = false;
			} else if(!FileSystem::hasAccessTo(coralExec,X_OK)) {
				messageFormatter->reportError("coral not executable (in " + coralRoot+"/bin)");
				ok = false;
			} else {
				messageFormatter->reportAction("Using coral [" + coralExec.getFilePath() + "]",VERBOSITY_SEARCHING);
			}
		}
		
		
		// Find svl executable (based on CADP environment variable)
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

		// Find an mrmc executable (based on PATH environment variable)
		{
			bool exists = false;
			bool accessible = false;
			vector<File> mrmcs;
			int n = FileSystem::findInPath(mrmcs,File("mrmc"));
			for(File mrmc: mrmcs) {
				accessible = false;
				exists = true;
				if(FileSystem::hasAccessTo(mrmc,X_OK)) {
					accessible = true;
					mrmcExec = mrmc;
					break;
				} else {
					messageFormatter->reportWarning("mrmc [" + mrmc.getFilePath() + "] is not runnable",VERBOSITY_SEARCHING);
					ok = false;
				}
			}
			if(!accessible) {
				messageFormatter->reportError("no runnable mrmc executable found in PATH");
				ok = false;
			} else {
				messageFormatter->reportAction("Using mrmc [" + mrmcExec.getFilePath() + "]",VERBOSITY_SEARCHING);
			}
		}
		
		// Find bcg_io executable (based on PATH environment variable)
		{
			bool exists = false;
			bool accessible = false;
			vector<File> bcgios;
			int n = FileSystem::findInPath(bcgios,File("bcg_io"));
			for(File bcgio: bcgios) {
				accessible = false;
				exists = true;
				if(FileSystem::hasAccessTo(bcgio,X_OK)) {
					accessible = true;
					bcgioExec = bcgio;
					break;
				} else {
					messageFormatter->reportWarning("bcg_io [" + bcgio.getFilePath() + "] is not runnable",VERBOSITY_SEARCHING);
					ok = false;
				}
			}
			if(!accessible) {
				messageFormatter->reportError("no runnable bcg_io executable found in PATH");
				ok = false;
			} else {
				messageFormatter->reportAction("Using bcg_io [" + bcgioExec.getFilePath() + "]",VERBOSITY_SEARCHING);
			}
		}

		// Find dot executable (based on PATH environment variable)
		{
			bool exists = false;
			bool accessible = false;
			vector<File> dots;
			int n = FileSystem::findInPath(dots,File("dot"));
			for(File dot: dots) {
				accessible = false;
				exists = true;
				if(FileSystem::hasAccessTo(dot,X_OK)) {
					accessible = true;
					dotExec = dot;
					break;
				} else {
					messageFormatter->reportWarning("dot [" + dot.getFilePath() + "] is not runnable",VERBOSITY_SEARCHING);
					ok = false;
				}
			}
			if(!accessible) {
				messageFormatter->reportError("no runnable dot executable found in PATH");
				ok = false;
			} else {
				messageFormatter->reportAction("Using dot [" + dotExec.getFilePath() + "]",VERBOSITY_SEARCHING);
			}
		}
		
		return !ok;
	}
	
	void setMessageFormatter(MessageFormatter* messageFormatter) {
		this->messageFormatter = messageFormatter;
	}
	void setBuildDOT(const std::string& buildDot) {
		this->buildDot = buildDot;
	}

	int printOutput(const File& file);
	int calculateDFT(const std::string& cwd, const File& dft, FileWriter& out);
	
};

#endif