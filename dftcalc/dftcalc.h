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
#include <vector>
#include "DFTCalculationResult.h"
#include "executor.h"

namespace DFT {
	extern const int VERBOSITY_FLOW;

	enum checker {STORM, MRMC, IMRMC, IMCA, MODEST, EXP_ONLY};
	enum converter {SVL, DFTRES};

	class DFTCalc {
	public:
		static const int VERBOSITY_SEARCHING;

		DFTCalc(MessageFormatter *mf)
			:messageFormatter(mf), exec(nullptr)
		{}

		~DFTCalc()
		{
		}
		
	private:
		
		/// The -T<buildDot> argument passed to dot
		std::string buildDot;
		
		MessageFormatter * const messageFormatter;
		std::string dft2lntRoot;
		std::string coralRoot;
		std::string imcaRoot;
		File dft2lntcExec;
#ifdef HAVE_CADP
		std::string cadpRoot;
		File imc2ctmdpExec;
		File bcg2imcaExec;
		File bcg2janiExec;
		File bcg2tralabExec;
		File maxprogExec;
		File svlExec;
		File bcgioExec;
		File bcginfoExec;
		File bcgminExec;
		std::string getCADPRoot();
#endif
		File stormExec;
		File mrmcExec;
		File imrmcExec;
		File imcaExec;
		File dotExec;
		File javaExec;
		File dftresJar;
		
		std::string getCoralRoot();
		std::string getImcaRoot();
		std::string getRoot();
		File getDftresJar();

		std::vector<std::string> evidence;
		std::unordered_map<std::string, DFTCalculationResult> cachedResults;
		CommandExecutor *exec;

		int checkModule(const bool reuse,
		                const std::string& cwd,
		                const File& dftOriginal,
		                const std::vector<Query> &queries,
		                enum DFT::checker useChecker,
		                enum DFT::converter useConverter,
		                bool warnNonDeterminism,
		                DFT::DFTCalculationResult &ret,
		                bool expOnly,
		                std::string &module);

		bool findInPath(std::string tool, File &ret);
	public:
		/**
		 * Verifies all the needed tools for calculation are installed.
		 * Will report errors to the MessageFormatter specified with 
		 * setMessageFormatter().
		 * @return false: all tools found OK, true: error
		 */
		bool checkNeededTools(DFT::checker checker, converter conv);
		
		/**
		 * Sets the image output format to be passed to dot as the -T argument.
		 * Example: png
		 * @param buildDot Image output format to be passed to dot.
		 */
		void setBuildDOT(const std::string& buildDot) {
			this->buildDot = buildDot;
		}
		
		/**
		 * Calculates the specified DFT file with modularization.
		 * @param reuse Whether to reuse intermediate files.
		 * @param cwd The directory to work in.
		 * @param dftOriginal The DFT to analyze.
		 * @param queries The queries to model check.
		 * @param useChecker Which model checker to use.
		 * @param useConverter Which converter to use in
		 * 	convertion from BCG to model checker format.
		 * @param warnNonDeterminism Whether to issue a warning
		 * 	if the BCG file contains nondeterminism.
		 * @param ret Will have the calculated result added.
		 * @return 0 if successful, non-zero otherwise
		 */
		int calcModular(const bool reuse,
		                const std::string& cwd,
		                const File& dftOriginal,
		                const std::vector<Query> &queries,
		                enum DFT::checker useChecker,
		                enum DFT::converter useConverter,
		                bool warnNonDeterminism,
		                DFT::DFTCalculationResult &ret,
		                bool expOnly);
		/**
		 * Calculates the specified DFT file.
		 * @param reuse Whether to reuse intermediate files.
		 * @param cwd The directory to work in.
		 * @param dftOriginal The DFT to analyze.
		 * @param queries The queries to model check.
		 * @param useChecker Which model checker to use.
		 * @param useConverter Which converter to use in
		 * 	convertion from BCG to model checker format.
		 * @param warnNonDeterminism Whether to issue a warning
		 * 	if the BCG file contains nondeterminism.
		 * @param root Which FT element to use as the root
		 *      (toplevel if empty).
		 * @param ret Will have the calculated result added.
		 * @return 0 if successful, non-zero otherwise
		 */
		int calculateDFT(const bool reuse,
		                 const std::string& cwd,
		                 const File& dftOriginal,
		                 const std::vector<Query> &queries,
		                 enum DFT::checker useChecker,
		                 enum DFT::converter useConverter,
		                 bool warnNonDeterminism,
		                 std::string root,
		                 DFT::DFTCalculationResult &ret,
		                 bool expOnly);

		void setEvidence(const std::vector<std::string>& evidence) {this->evidence = evidence;}
		const std::vector<std::string>& getEvidence() const {return evidence;}
	};

} // Namespace: DFT

#endif
