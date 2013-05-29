#ifndef ANALYSIS_STATE_H
#define ANALYSIS_STATE_H

#include <string>

/**
 * The ProcessingState enum describes the result of a
 * FunctionProcessor run.
 */
enum ProcessingState {
  Success,
  ErrorMissingDefinition,
  ErrorMissingIntrinsic,
  ErrorArguments,
  Error,
  Deferred
};


class AnalysisState {
  public:
    AnalysisState() {
        _state = Success;
        _canceled = false;
        _missingDefinition = NULL;
    }

    bool isCanceled() const { return _canceled; }

    void setCanceled() { _canceled = true; }

    ProcessingState getProcessingState() const { return _state; }

    void setProcessingState(const ProcessingState &state) { _state = state; }

    void setMissingDefinition(const Function* f) { _missingDefinition = const_cast<Function*>(f); }

    const Function* getMissingDefinition() { return _missingDefinition; }

    void stopWithError(string msg, ProcessingState state = Error) {
      setCanceled();
      setProcessingState(state);

      //if (msg.str().length())
        //ERROR_LOG(msg << "\n");
    }

private:
    ProcessingState _state;
    bool _canceled;
    Function* _missingDefinition;
};

#endif // ANALYSIS_STATE_H
