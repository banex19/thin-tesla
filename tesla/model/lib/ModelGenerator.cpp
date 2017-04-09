#include "Debug.h"
#include "ModelGenerator.h"

std::string ModelGenerator::NextLabel() {
  return "_s" + std::to_string(label++);
}

set<ModelGenerator::Model> ModelGenerator::ofLength(size_t length) {
  return fromExpression(Expr, length);
}

set<ModelGenerator::Model> ModelGenerator::fromExpression(const Expression &ex, size_t length) {
  switch(ex.type()) {
    case Expression_Type_BOOLEAN_EXPR:
      return fromBoolean(ex.booleanexpr(), length);

    case Expression_Type_SEQUENCE:
      return fromSequence(ex.sequence(), length);

    case Expression_Type_NULL_EXPR:
      return {{}};

    case Expression_Type_ASSERTION_SITE:
      return fromAssertionSite(ex.assertsite(), length);

    case Expression_Type_FUNCTION:
      return fromFunction(ex.function(), length);

    case Expression_Type_FIELD_ASSIGN:
      return fromFieldAssign(ex.fieldassign(), length);

    case Expression_Type_SUB_AUTOMATON:
      auto sub = Man->FindAutomaton(ex.subautomaton());
      return fromSubAutomaton(*sub, length);
  }
}

/**
 * What do we need to do here? We can generate every possible model from the
 * subexpressions of this sequence, and we need to combine them into a sequenced
 * model with an appropriate length.
 *
 * Could implement a variant that takes a length parameter and goes recursively.
 * If we can take a model of length n at the start and length - n at the end,
 * then concatenate them and return.
 */
set<ModelGenerator::Model> ModelGenerator::fromSequence(const Sequence &ex, size_t length) {
  return fromSequence(ex, length, 0);
}

set<ModelGenerator::Model> ModelGenerator::fromSequence(const Sequence &ex, size_t length, size_t index) {
  if(length == 0) {
    return {};
  }

  if(index >= ex.expression_size()) {
    return {{}};
  }

  const auto& head = ex.expression(index);
  const auto& hes = fromExpression(head, length);

  set<ModelGenerator::Model> ret;

  for(const auto& he : hes) {
    const auto& tes = fromSequence(ex, length - he.size(), index+1);
    
    for(const auto& te : tes) {
      ModelGenerator::Model cat;
      for(const auto& hev : he) {
        cat.push_back(hev);
      }

      for(const auto& tev : te) {
        cat.push_back(tev);
      }

      if(cat.size() <= length && !cat.empty()) {
        const auto& once = cat;
        const auto rep_bound = length / cat.size();

        for(auto i = ex.minreps(); i <= ex.maxreps() && i <= rep_bound; i++) {
          ModelGenerator::Model repd;
          for(int j = 0; j < i; j++) {
            std::copy(once.begin(), once.end(), std::back_inserter(repd));
          }
          ret.insert(repd);
        }
      }
    }
  }

  return ret;
}

/**
 * The only boolean expression we care about is inclusive OR - any model
 * generated by one of its subexpressions is valid, as long as the length
 * restriction is maintained.
 */
set<ModelGenerator::Model> ModelGenerator::fromBoolean(const BooleanExpr &ex, size_t length) {
  assert(ex.operation() == BooleanExpr_Operation_BE_Xor);

  set<ModelGenerator::Model> possibles;
  for(int i = 0; i < ex.expression_size(); i++) {
    const auto& disj = fromExpression(ex.expression(i), length);
    std::copy(disj.begin(), disj.end(), std::inserter(possibles, possibles.begin()));
  }

  return possibles;
}

/**
 * An assertion site event can only generate one possible assertion (itself).
 */
set<ModelGenerator::Model> ModelGenerator::fromAssertionSite(const AssertionSite &ex, size_t length) {
  if(length == 0) {
    return {};
  }

  auto expr = new Expression;
  expr->set_type(Expression_Type_ASSERTION_SITE);
  *expr->mutable_assertsite() = ex;
  return {{expr}};
}

/**
 * A function event can only generate one possible assertion (itself).
 */
set<ModelGenerator::Model> ModelGenerator::fromFunction(const FunctionEvent &ex, size_t length) {
  if(length == 0) {
    return {};
  }

  auto expr = new Expression;
  expr->set_type(Expression_Type_FUNCTION);
  *expr->mutable_function() = ex;
  return {{expr}};
}

/**
 * Needs to return a field assignment event rather than just the empty set
 * because we cannot check field assignment events at all - so we need to mark
 * that by having the event in the generated model.
 */
set<ModelGenerator::Model> ModelGenerator::fromFieldAssign(const FieldAssignment &ex, size_t length) {
  if(length == 0) {
    return {};
  }

  auto expr = new Expression;
  expr->set_type(Expression_Type_FIELD_ASSIGN);
  *expr->mutable_fieldassign() = ex;
  return {{expr}};
}

/**
 * Just recurse directly into the named subautomaton.
 */
set<ModelGenerator::Model> ModelGenerator::fromSubAutomaton(const Automaton &ex, size_t length) {
  return fromExpression(ex.getAssertion().expression(), length);
}

FiniteStateMachine<Expression *> ModelGenerator::ExpressionFSM(const Expression &ex) {
  switch(ex.type()) {
    case Expression_Type_NULL_EXPR:
      return NullFSM();

    case Expression_Type_BOOLEAN_EXPR:
      return BooleanFSM(ex.booleanexpr());

    case Expression_Type_SEQUENCE:
      return SequenceFSM(ex.sequence());

    case Expression_Type_ASSERTION_SITE:
      return AssertionSiteFSM(ex.assertsite());

    case Expression_Type_FUNCTION:
      return FunctionEventFSM(ex.function());

    case Expression_Type_FIELD_ASSIGN:
      return FieldAssignFSM(ex.fieldassign());

    case Expression_Type_SUB_AUTOMATON:
      auto sub = Man->FindAutomaton(ex.subautomaton());
      return SubAutomatonFSM(*sub);
  }
}

FiniteStateMachine<Expression *> ModelGenerator::BooleanFSM(const BooleanExpr &ex) {
  auto fsm = FiniteStateMachine<Expression *>{};

  auto initial_state = ::State{NextLabel()};
  initial_state.initial = true;

  auto accept_state = ::State{NextLabel()};
  accept_state.accepting = true;

  auto initial_added = fsm.AddState(initial_state);
  auto accept_added = fsm.AddState(accept_state);

  for(auto i = 0; i < ex.expression_size(); i++) {
    auto sub_fsm = ExpressionFSM(ex.expression(i));

    auto& sub_added = fsm.AddSubMachine(sub_fsm);
    fsm.AddEdge(initial_added, sub_added.InitialState());
    sub_added.InitialState()->initial = false;

    for(const auto& sub_accept : sub_added.AcceptingStates()) {
      fsm.AddEdge(sub_accept, accept_added);
      sub_accept->accepting = false;
    }
  }

  if(ex.expression_size() == 0) {
    fsm.AddEdge(initial_added, accept_added);
  }

  return fsm;
}

FiniteStateMachine<Expression *> ModelGenerator::SequenceOnceFSM(const Sequence &ex) {
  auto fsm = FiniteStateMachine<Expression *>{};

  auto initial_state = ::State{NextLabel()};
  initial_state.initial = true;

  auto accept_state = ::State{NextLabel()};
  accept_state.accepting = true;

  auto initial_added = fsm.AddState(initial_state);
  auto accept_added = fsm.AddState(accept_state);

  auto tails = std::set<std::shared_ptr<::State>>{ initial_added };

  for(auto i = 0; i < ex.expression_size(); i++) {
    auto sub_fsm = ExpressionFSM(ex.expression(i));
    auto& sub_added = fsm.AddSubMachine(sub_fsm);

    for(const auto& accept : tails) {
      accept->accepting = false;
      fsm.AddEdge(accept, sub_added.InitialState());
    }

    sub_added.InitialState()->initial = false;

    tails = sub_added.AcceptingStates();
  }

  for(const auto& accept : tails) {
    accept->accepting = false;
    fsm.AddEdge(accept, accept_added);
  }
 
  initial_added->initial = true;
  return fsm;
}

FiniteStateMachine<Expression *> ModelGenerator::SequenceFSM(const Sequence &ex) {
  auto fsm = FiniteStateMachine<Expression *>{};

  auto initial_state = ::State{NextLabel()};
  initial_state.initial = true;

  auto accept_state = ::State{NextLabel()};
  accept_state.accepting = true;

  auto initial_added = fsm.AddState(initial_state);
  auto accept_added = fsm.AddState(accept_state);

  auto tails = std::set<std::shared_ptr<::State>>{ initial_added };

  if(ex.maxreps() == std::numeric_limits<int>::max()) {
    for(auto i = 0; i < ex.minreps(); i++) {
      auto rep = SequenceOnceFSM(ex);
      auto& rep_added = fsm.AddSubMachine(rep);

      for(const auto& accept : tails) {
        accept->accepting = false;
        fsm.AddEdge(accept, rep_added.InitialState());
      }

      rep_added.InitialState()->initial = false;
      tails = rep_added.AcceptingStates();
    }

    fsm.AddEdge(accept_added, initial_added);
  } else {
    for(auto i = 0; i < ex.maxreps(); i++) {
      auto rep = SequenceOnceFSM(ex);
      auto& rep_added = fsm.AddSubMachine(rep);

      for(const auto& accept : tails) {
        accept->accepting = false;
        fsm.AddEdge(accept, rep_added.InitialState());

        if(i > ex.minreps()) {
          for(const auto& rep_accept : rep_added.AcceptingStates()) {
            fsm.AddEdge(rep_accept, accept_added);
          }
        }
      }

      rep_added.InitialState()->initial = false;

      tails = rep_added.AcceptingStates();
    }
  }

  for(const auto& accept : tails) {
    accept->accepting = false;
    fsm.AddEdge(accept, accept_added);
  }

  initial_added->initial = true;
  return fsm;
}

FiniteStateMachine<Expression *> ModelGenerator::AssertionSiteFSM(const AssertionSite &ex) {
  auto fsm = FiniteStateMachine<Expression *>{};

  auto initial_state = ::State{NextLabel()};
  initial_state.initial = true;

  auto accept_state = ::State{NextLabel()};
  accept_state.accepting = true;

  auto initial_added = fsm.AddState(initial_state);
  auto accept_added = fsm.AddState(accept_state);

  auto expr = new Expression;
  expr->set_type(Expression_Type_ASSERTION_SITE);
  *expr->mutable_assertsite() = ex;

  fsm.AddEdge(initial_added, accept_added, expr);

  return fsm;
}

FiniteStateMachine<Expression *> ModelGenerator::FunctionEventFSM(const FunctionEvent &ex) {
  auto fsm = FiniteStateMachine<Expression *>{};

  auto initial_state = ::State{NextLabel()};
  initial_state.initial = true;

  auto accept_state = ::State{NextLabel()};
  accept_state.accepting = true;

  auto initial_added = fsm.AddState(initial_state);
  auto accept_added = fsm.AddState(accept_state);

  auto expr = new Expression;
  expr->set_type(Expression_Type_FUNCTION);
  *expr->mutable_function() = ex;

  fsm.AddEdge(initial_added, accept_added, expr);

  return fsm;
}

FiniteStateMachine<Expression *> ModelGenerator::FieldAssignFSM(const FieldAssignment &ex) {
  auto fsm = FiniteStateMachine<Expression *>{};

  auto initial_state = ::State{NextLabel()};
  initial_state.initial = true;

  auto accept_state = ::State{NextLabel()};
  accept_state.accepting = true;

  auto initial_added = fsm.AddState(initial_state);
  auto accept_added = fsm.AddState(accept_state);

  auto expr = new Expression;
  expr->set_type(Expression_Type_FIELD_ASSIGN);
  *expr->mutable_fieldassign() = ex;

  fsm.AddEdge(initial_added, accept_added, expr);

  return fsm;
}

FiniteStateMachine<Expression *> ModelGenerator::SubAutomatonFSM(const Automaton &ex) {
  return ExpressionFSM(ex.getAssertion().expression());
}

FiniteStateMachine<Expression *> ModelGenerator::NullFSM() {
  auto fsm = FiniteStateMachine<Expression *>{};
  auto accept = ::State{NextLabel()};
  accept.accepting = true;
  accept.initial = true;

  fsm.AddState(accept);
  return fsm;
}
