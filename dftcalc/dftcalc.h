/*
 * dftcalc.h
 * 
 * Part of dft2lnt library - a library containing read/write operations for DFT
 * files in Galileo format and translating DFT specifications into Lotos NT.
 * 
 * @author Freark van der Berg, extended by Dennis Guck
 */

#ifndef DFTCALC_H
#define DFTCALC_H

#include <string>
#include <unordered_map>
#include "DFTCalculationResult.h"

namespace DFT {

	class DFTCalc {
	public:
		static const int VERBOSITY_SEARCHING;
		
	private:
		
		/// The -T<buildDot> argument passed to dot
		std::string buildDot;
		
		MessageFormatter* messageFormatter;
		std::string dft2lntRoot;
		std::string coralRoot;
		std::string imcaRoot;
		std::string cadpRoot;
		File dft2lntcExec;
		File coralExec;
		File imc2ctmdpExec;
		File bcg2imcaExec;
		File svlExec;
		File bcgioExec;
		File mrmcExec;
		File imcaExec;
		File dotExec;
		
		std::string getCoralRoot(MessageFormatter* messageFormatter);
		std::string getImcaRoot(MessageFormatter* messageFormatter);
		std::string getRoot(MessageFormatter* messageFormatter);
		std::string getCADPRoot(MessageFormatter* messageFormatter);
		
		/// A map containing the results of the calculation. <filename> --> <result>
		map<std::string,DFT::DFTCalculationResult> results;
		
		std::vector<std::string> evidence;
		
	public:
		
		/**
		 * Returns the result map. The result map contains the mapping:
		 *   <filename> --> <result>
		 * @return The result map.
		 */
		const map<std::string,DFT::DFTCalculationResult> getResults() const {
			return results;
		}
		
		/**
		 * Verifies all the needed tools for calculation are installed.
		 * Will report errors to the MessageFormatter specified with 
		 * setMessageFormatter().
		 * @return false: all tools found OK, true: error
		 */
		bool checkNeededTools() {
			
			bool ok = true;
			
			messageFormatter->notify("Checking environment...",VERBOSITY_SEARCHING);
			
			/* Obtain all needed root information from environment */
			dft2lntRoot = getRoot(NULL);
			coralRoot = getCoralRoot(NULL);
			cadpRoot = getCADPRoot(NULL);
			imcaRoot = getImcaRoot(NULL);
			
			/* These tools should be easy to find in the roots. Note that an added
			 * bonus would be to locate them using PATH as well.
			 */
			dft2lntcExec = File(dft2lntRoot+"/bin/dft2lntc");
			imc2ctmdpExec = File(coralRoot+"/bin/imc2ctmdp");
			bcg2imcaExec = File(imcaRoot+"/bin/bcg2imca");
			coralExec = File(coralRoot+"/coral");
			svlExec = File(cadpRoot+"/com/svl");
			
			/* Find dft2lntc executable (based on DFT2LNTROOT environment variable) */
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
			
			/* Find imc2ctmdpi and coral executables (based on CORAL environment variable) */
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
				if(!FileSystem::hasAccessTo(bcg2imcaExec,F_OK)) {
					messageFormatter->reportError("bcg2imca not found (in " + imcaRoot+"/bin)");
					ok = false;
				} else if(!FileSystem::hasAccessTo(bcg2imcaExec,X_OK)) {
					messageFormatter->reportError("bcg2imca not executable (in " + imcaRoot+"/bin)");
					ok = false;
				} else {
					messageFormatter->reportAction("Using bcg2imca [" + bcg2imcaExec.getFilePath() + "]",VERBOSITY_SEARCHING);
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
			
			/* Find an mrmc executable (based on PATH environment variable) */
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
			
			/* Find an imca executable (based on PATH environment variable) */
			{
				bool exists = false;
				bool accessible = false;
				vector<File> imcas;
				int n = FileSystem::findInPath(imcas,File("imca"));
				for(File imca: imcas) {
					accessible = false;
					exists = true;
					if(FileSystem::hasAccessTo(imca,X_OK)) {
						accessible = true;
						imcaExec = imca;
						break;
					} else {
						messageFormatter->reportWarning("imca [" + imca.getFilePath() + "] is not runnable",VERBOSITY_SEARCHING);
						ok = false;
					}
				}
				if(!accessible) {
					messageFormatter->reportError("no runnable imca executable found in PATH");
					ok = false;
				} else {
					messageFormatter->reportAction("Using imca [" + imcaExec.getFilePath() + "]",VERBOSITY_SEARCHING);
				}
			}
			
			/* Find bcg_io executable (based on PATH environment variable) */
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
			
			/* Find dot executable (based on PATH environment variable) */
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
		
		/**
		 * Sets the MessageFormatter to be used by this DFTCalc instance.
		 * @param messageFormatter The MessageFormatter to be used by this DFTCalc instance
		 */
		void setMessageFormatter(MessageFormatter* messageFormatter) {
			this->messageFormatter = messageFormatter;
		}
		
		/**
		 * Sets the image output format to be passed to dot as the -T argument.
		 * Example: png
		 * @param buildDot Image output format to be passed to dot.
		 */
		void setBuildDOT(const std::string& buildDot) {
			this->buildDot = buildDot;
		}
		
		/**
		 * Prints the contents of the specified File to the messageFormatter specified
		 * with setMessageFormatter().
		 * @param file The file to print.
		 */
		void printOutput(const File& file, int status);
		
		/**
		 * Calculates the specified DFT file, using the specified working directory.
		 * @param cwd The directory to work in.
		 * @param dft The DFT to calculate
		 * @return 0 if successful, non-zero otherwise
		 */
		int calculateDFT(const bool reuse, const std::string& cwd, const File& dft, const std::vector<std::pair<std::string,std::string>>& timeSpec, unordered_map<string,string> settings,  bool calcImca);
		
		void setEvidence(const std::vector<std::string>& evidence) {this->evidence = evidence;}
		const std::vector<std::string>& getEvidence() const {return evidence;}
	};

} // Namespace: DFT

#endif
