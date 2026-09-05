#include "arena.h"
#include "base.h"
#include <string.h>

// autograd automatically computes the gradients of the loss function with
// respect to the neural network’s weights and biases.

#define NUM_OF_INPUTS 2

typedef struct Var Var;
typedef struct matrix matrix;
typedef struct VarType VarType;
typedef struct model_state model_state;
typedef enum FLAGS FLAGS;
typedef struct Graph Graph;

typedef b32 VarShapeFunc(Var *a, Var *b, u32 *cols, u32 *rows);
typedef b32 VarFunc(Var *var);

struct matrix {
  u32 rows;
  u32 cols;

  f32 *data;
};

enum FLAGS {
  VAR_FLAG_NONE = 0,
  VAR_FLAG_REQUIRES_GRAD = (1 << 0),
  VAR_FLAG_PARAMETER = (1 << 1)
};

struct VarType {
  const char *name;
  u32 num_inputs;
  VarShapeFunc *shape;
  VarFunc *forward;
  VarFunc *backward;
};

struct Var {
  u32 index;
  u32 flags;

  matrix *val;
  matrix *grad;

  Var *inputs[NUM_OF_INPUTS];

  VarType *type;
};

struct model_state {
  u32 num_vars;
  f32 advantage;
};

struct Graph {
  Var **vars;
  u32 size;
};

matrix *create_matrix(mem_arena *arena, u32 rows, u32 cols) {
  matrix *out = PUSH_STRUCT(arena, matrix);

  out->rows = rows;
  out->cols = cols;

  out->data = arena_push(arena, sizeof(matrix) * (rows * cols), true);

  return out;
}

Var *create_var(mem_arena *arena, model_state *model, u32 rows, u32 cols,
                u32 flags) {
  Var *out = PUSH_STRUCT(arena, Var);
  out->index = model->num_vars++;
  out->type = NULL;

  matrix *val = create_matrix(arena, rows, cols);

  out->val = val;
  if (out->flags & VAR_FLAG_REQUIRES_GRAD) {
    out->grad = create_matrix(arena, rows, cols);
  }

  return out;
}

Var *create_node(mem_arena *arena, model_state *model, Var *a, Var *b,
                 VarType *type) {

  if (type == NULL) {
    return NULL;
  }

  u32 flag = VAR_FLAG_NONE;

  if (a != NULL && (a->flags & VAR_FLAG_REQUIRES_GRAD) ||
      b != NULL && (b->flags & VAR_FLAG_REQUIRES_GRAD)) {
    flag = VAR_FLAG_REQUIRES_GRAD;
  }

  u32 rows = 0;
  u32 cols = 0;
  if (!type->shape(a, b, &cols, &rows)) {
    return NULL;
  }

  Var *var = create_var(arena, model, rows, cols, flag);
  var->type = type;

  var->inputs[0] = a;
  if (var->type->num_inputs > 1) {
    var->inputs[1] = b;
  }

  return var;
}

Var *var_softmax(mem_arena *arena, model_state *model, VarType type,
                 Var *input);
Var *var_relu(mem_arena *arena, model_state *model, VarType type, Var *input);
Var *var_add(mem_arena *arena, model_state *model, VarType type, Var *a,
             Var *b);
Var *var_sub(mem_arena *arena, model_state *model, VarType type, Var *a,
             Var *b);
Var *var_matmul(mem_arena *arena, model_state *model, VarType type, Var *a,
                Var *b);
Var *var_reinforce(mem_arena *arena, model_state *model, VarType type,
                   Var *probs, Var *rt);

Var *var_softmax_forward(mem_arena *arena, model_state *model, VarType type,
                         Var *input);
Var *var_relu_forward(mem_arena *arena, model_state *model, VarType type,
                      Var *input);
Var *var_add_forward(mem_arena *arena, model_state *model, VarType type, Var *a,
                     Var *b);
Var *var_sub_forward(mem_arena *arena, model_state *model, VarType type, Var *a,
                     Var *b);
Var *var_matmul_forward(mem_arena *arena, model_state *model, VarType type,
                        Var *a, Var *b);
Var *var_reinforce_forward(mem_arena *arena, model_state *model, VarType type,
                           Var *probs, Var *rt);

Var *var_softmax_backward(Var *var);
Var *var_relu_backward(Var *var);
Var *var_add_backward(Var *var);
Var *var_sub_backward(Var *var);
Var *var_matmul_backward(Var *var);
Var *var_reinforce_backward(Var *var);

int main() {

  mem_arena *arena = arena_create(1000);
  model_state model = {0};

  Var *b = create_var(arena, &model, 20, 20, 0);
  printf("Var b: %u\n", b->val->rows);

  return 0;
}
