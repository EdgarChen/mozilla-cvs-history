;;; The contents of this file are subject to the Mozilla Public
;;; License Version 1.1 (the "License"); you may not use this file
;;; except in compliance with the License. You may obtain a copy of
;;; the License at http://www.mozilla.org/MPL/
;;; 
;;; Software distributed under the License is distributed on an "AS
;;; IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
;;; implied. See the License for the specific language governing
;;; rights and limitations under the License.
;;; 
;;; The Original Code is the Language Design and Prototyping Environment.
;;; 
;;; The Initial Developer of the Original Code is Netscape Communications
;;; Corporation.  Portions created by Netscape Communications Corporation are
;;; Copyright (C) 1999 Netscape Communications Corporation.  All
;;; Rights Reserved.
;;; 
;;; Contributor(s):   Waldemar Horwat <waldemar@acm.org>

;;;
;;; ECMAScript semantic calculus
;;;
;;; Waldemar Horwat (waldemar@acm.org)
;;;

(declaim (optimize (debug 3))) ;*****
(defvar *trace-variables* nil)


#+mcl (dolist (indent-spec '((? . 1) (apply . 1) (funcall . 1) (production . 3) (rule . 2) (function . 2)
                             (deftag . 1) (defrecord . 1) (deftype . 1) (tag . 1) (%text . 1)
                             (var . 2) (const . 2) (rwhen . 1) (while . 1) (:narrow . 1) (:select . 1)))
        (pushnew indent-spec ccl:*fred-special-indent-alist* :test #'equal))


; Return the boolean exclusive or of the arguments.
(defun xor (&rest as)
  (let ((result nil))
    (dolist (a as)
      (when a
        (setq result (not result))))
    result))


; A boolean version of = that works on any nil/non-nil values.
(declaim (inline boolean=))
(defun boolean= (a b)
  (eq (not a) (not b)))


; Complement of eq.
(declaim (inline not-eq))
(defun not-eq (a b)
  (not (eq a b)))


(defun digit-char-36 (char)
  (assert-non-null (digit-char-p char 36)))


; Call map on each element of the list l.  If map returns true, call filter on that element.  Gather the results
; of the calls to filter into a new list and return that list.
(defun filter-map-list (filter map l)
  (let ((results nil))
    (dolist (e l)
      (when (funcall filter e)
        (push (funcall map e) results)))
    (nreverse results)))

; Call map on each element of the sequence s.  If map returns true, call filter on that element.  Gather the results
; of the calls to filter into a new sequence of type result-type and return that sequence.
(defun filter-map (result-type filter map s)
  (let ((results nil))
    (map nil
         #'(lambda (e)
             (when (funcall filter e)
               (push (funcall map e) results)))
         s)
    (coerce result-type (nreverse results))))


; Return the same symbol in the keyword package.
(defun find-keyword (symbol)
  (assert-non-null (find-symbol (string symbol) (find-package :keyword))))


;;; ------------------------------------------------------------------------------------------------------
;;; DOUBLE-PRECISION FLOATING-POINT NUMBERS

(deftype float64 ()
         '(or (and float (not (eql 0.0)) (not (eql -0.0))) (member :+zero :-zero :+infinity :-infinity :nan)))

(defun float64? (n)
  (or (and (floatp n) (not (zerop n)))
      (member n '(:+zero :-zero :+infinity :-infinity :nan))))

; Evaluate expr.  If it evaluates successfully, return its value except if it evaluates to
; +0.0 or -0.0, in which case return :+zero (but not :-zero).
; If evaluating expr overflows, evaluate sign; if it returns a positive value, return :+infinity;
; otherwise return :-infinity.  sign should not return zero.
(defmacro handle-overflow (expr &body sign)
  (let ((x (gensym)))
    `(handler-case (let ((,x ,expr))
                     (if (zerop ,x) :+zero ,x))
       (floating-point-overflow () (if (minusp (progn ,@sign)) :-infinity :+infinity)))))


(defun rational-to-float64 (r)
  (let ((f (handle-overflow (coerce r 'double-float)
             r)))
    (if (eq f :+zero)
      (if (minusp r) :-zero :+zero)
      f)))


; Return true if n is +0 or -0 and false otherwise.
(declaim (inline float64-is-zero))
(defun float64-is-zero (n)
  (or (eq n :+zero) (eq n :-zero)))


; Return true if n is NaN and false otherwise.
(declaim (inline float64-is-nan))
(defun float64-is-nan (n)
  (eq n :nan))


; Return true if n is :+infinity or :-infinity and false otherwise.
(declaim (inline float64-is-infinite))
(defun float64-is-infinite (n)
  (or (eq n :+infinity) (eq n :-infinity)))


; Truncate n to the next lower integer.  Signal an error if n isn't finite.
(defun truncate-finite-float64 (n)
  (if (float64-is-zero n)
    0
    (truncate n)))


; Return:
;   :less if n<m;
;   :equal if n=m;
;   :greater if n>m;
;   :unordered if either n or m is :nan.
(defun float64-compare (n m)
  (when (float64-is-zero n)
    (setq n 0.0))
  (when (float64-is-zero m)
    (setq m 0.0))
  (cond
   ((or (float64-is-nan n) (float64-is-nan m)) :unordered)
   ((eql n m) :equal)
   ((or (eq n :+infinity) (eq m :-infinity)) :greater)
   ((or (eq m :+infinity) (eq n :-infinity)) :less)
   ((< n m) :less)
   ((> n m) :greater)
   (t :equal)))


; Return
;    1 if n is +0.0, :+infinity, or any positive floating-point number;
;   -1 if n is -0.0, :-infinity, or any positive floating-point number;
;    0 if n is :nan.
(defun float64-sign (n)
  (case n
    ((:+zero :+infinity) 1)
    ((:-zero :-infinity) -1)
    (:nan 0)
    (t (round (float-sign n)))))


; Return
;   0 if either n or m is :nan;
;   1 if n and m have the same float64-sign;
;  -1 if n and m have different float64-signs.
(defun float64-sign-xor (n m)
  (* (float64-sign n) (float64-sign m)))


; Return d truncated towards zero into a 32-bit integer.  Overflows wrap around.
(defun float64-to-uint32 (d)
  (case d
    ((:+zero :-zero :+infinity :-infinity :nan) 0)
    (t (mod (truncate d) #x100000000))))


; Return the absolute value of n.
(defun float64-abs (n)
  (case n
    ((:+zero :-zero) :+zero)
    ((:+infinity :-infinity) :+infinity)
    (:nan :nan)
    (t (abs n))))


; Return -n.
(defun float64-neg (n)
  (case n
    (:+zero :-zero)
    (:-zero :+zero)
    (:+infinity :-infinity)
    (:-infinity :+infinity)
    (:nan :nan)
    (t (- n))))


; Return n+m.
(defun float64-add (n m)
  (case n
    (:+zero (if (eq m :-zero) :+zero m))
    (:-zero m)
    (:+infinity (case m
                  ((:-infinity :nan) :nan)
                  (t :+infinity)))
    (:-infinity (case m
                  ((:+infinity :nan) :nan)
                  (t :-infinity)))
    (:nan :nan)
    (t (case m
         ((:+zero :-zero) n)
         (:+infinity :+infinity)
         (:-infinity :-infinity)
         (:nan :nan)
         (t (handle-overflow (+ n m)
              (let ((n-sign (float-sign n))
                    (m-sign (float-sign m)))
                (assert-true (= n-sign m-sign)) ;If the signs are opposite, we can't overflow.
                n-sign)))))))


; Return n-m.
(defun float64-subtract (n m)
  (float64-add n (float64-neg m)))


; Return n*m.
(defun float64-multiply (n m)
  (let ((sign (float64-sign-xor n m))
        (n (float64-abs n))
        (m (float64-abs m)))
    (let ((result (cond
                   ((zerop sign) :nan)
                   ((eq n :+infinity) (if (eq m :+zero) :nan :+infinity))
                   ((eq m :+infinity) (if (eq n :+zero) :nan :+infinity))
                   ((or (eq n :+zero) (eq m :+zero)) :+zero)
                   (t (handle-overflow (* n m) 1)))))
      (if (minusp sign)
        (float64-neg result)
        result))))


; Return n/m.
(defun float64-divide (n m)
  (let ((sign (float64-sign-xor n m))
        (n (float64-abs n))
        (m (float64-abs m)))
    (let ((result (cond
                   ((zerop sign) :nan)
                   ((eq n :+infinity) (if (eq m :+infinity) :nan :+infinity))
                   ((eq m :+infinity) :+zero)
                   ((eq m :+zero) (if (eq n :+zero) :nan :+infinity))
                   ((eq n :+zero) :+zero)
                   (t (handle-overflow (/ n m) 1)))))
      (if (minusp sign)
        (float64-neg result)
        result))))


; Return n%m, using the ECMAScript definition of %.
(defun float64-remainder (n m)
  (cond
   ((or (float64-is-nan n) (float64-is-nan m) (float64-is-infinite n) (float64-is-zero m)) :nan)
   ((or (float64-is-infinite m) (float64-is-zero n)) n)
   (t (let ((result (float (rem (rational n) (rational m)))))
        (if (zerop result)
          (if (minusp n) :-zero :+zero)
          result)))))


;;; ------------------------------------------------------------------------------------------------------
;;; SET UTILITIES

(defun integer-set-min (intset)
  (or (intset-min intset)
      (error "min of empty integer-set")))

(defun character-set-min (intset)
  (code-char (or (intset-min intset)
                 (error "min of empty character-set"))))


(defun integer-set-max (intset)
  (or (intset-max intset)
      (error "max of empty integer-set")))

(defun character-set-max (intset)
  (code-char (or (intset-max intset)
                 (error "max of empty character-set"))))


(defun integer-set-member (elt intset)
  (intset-member? intset elt))

(defun character-set-member (elt intset)
  (intset-member? intset (char-code elt)))


;;; ------------------------------------------------------------------------------------------------------
;;; CODE GENERATION

#+mcl(defvar *deferred-functions*)

(defun quiet-compile (name definition)
  #-mcl(compile name definition)
  #+mcl(handler-bind ((ccl::undefined-function-reference
                       #'(lambda (condition)
                           (setq *deferred-functions* (append (slot-value condition 'ccl::args) *deferred-functions*))
                           (muffle-warning condition))))
         (compile name definition)))


(defmacro defer-mcl-warnings (&body body)
  #-mcl`(with-compilation-unit () ,@body)
  #+mcl`(let ((*deferred-functions* nil))
          (multiple-value-prog1
            (with-compilation-unit () ,@body)
            (let ((missing-functions (remove-if #'fboundp *deferred-functions*)))
              (when missing-functions
                (warn "Undefined functions: ~S" missing-functions))))))


; If args has no elements, return the value of empty.
; If args has one element, return that element.
; If args has two or more elements, return (op . args).
(defun gen-poly-op (op empty args)
  (cond
   ((endp args) empty)
   ((endp (cdr args)) (car args))
   (t (cons op args))))


; Return `(progn ,@statements), optimizing where possible.
(defun gen-progn (statements)
  (cond
   ((endp statements) nil)
   ((and (endp (cdr statements))
         (let ((first-statement (first statements)))
           (not (and (consp first-statement)
                     (eq (first first-statement) 'declare)))))
    (first statements))
   (t (cons 'progn statements))))


; Return (nth <n> <code>), optimizing if possible.
(defun gen-nth-code (n code)
  (let ((abbrev (assoc n '((0 . first) (1 . second) (2 . third) (3 . fourth) (4 . fifth) (5 . sixth) (6 . seventh) (7 . eighth) (8 . ninth) (9 . tenth)))))
    (if abbrev
      (list (cdr abbrev) code)
      (list 'nth n code))))


; Return code that tests whether the result of evaluating code is a member of the given
; list of symbols using the test eq.
(defun gen-member-test (code symbols)
  (assert-true symbols)
  (if (cdr symbols)
    (list 'member code (list 'quote symbols) :test '#'eq)
    (list 'eq code (let ((symbol (car symbols)))
                     (if (constantp symbol)
                       symbol
                       (list 'quote symbol))))))


; Return `(funcall ,function-value ,@arg-values), optimizing where possible.
(defun gen-apply (function-value &rest arg-values)
  (let ((stripped-function-value (simple-strip-function function-value)))
    (cond
     (stripped-function-value
      (if (and (consp stripped-function-value)
               (eq (first stripped-function-value) 'lambda)
               (listp (second stripped-function-value))
               (cddr stripped-function-value)
               (every #'(lambda (arg)
                          (and (identifier? arg)
                               (not (eql (first-symbol-char arg) #\&))))
                      (second stripped-function-value)))
        (let ((function-args (second stripped-function-value))
              (function-body (cddr stripped-function-value)))
          (assert-true (= (length function-args) (length arg-values)))
          (if function-args
            (list* 'let
                   (mapcar #'list function-args arg-values)
                   function-body)
            (gen-progn function-body)))
        (cons stripped-function-value arg-values)))
     ((and (consp function-value)
           (eq (first function-value) 'symbol-function)
           (null (cddr function-value))
           (consp (cadr function-value))
           (eq (caadr function-value) 'quote)
           (identifier? (cadadr function-value))
           (null (cddadr function-value)))
      (cons (cadadr function-value) arg-values))
     (t (list* 'funcall function-value arg-values)))))


; Return `#'(lambda ,args (declare (ignore-if-unused ,@args)) ,body-code), optimizing
; where possible.
(defun gen-lambda (args body-code)
  (if args
    `#'(lambda ,args (declare (ignore-if-unused . ,args)) ,body-code)
    `#'(lambda () ,body-code)))


; If expr is a lambda-expression, return an equivalent expression that has
; the given name (which may be a symbol or a string; if it's a string, it is interned
; in the given package).  Otherwise, return expr unchanged.
; Attaching a name to lambda-expressions helps in debugging code by identifying
; functions in debugger backtraces.
(defun name-lambda (expr name &optional package)
  (if (and (consp expr)
           (eq (first expr) 'function)
           (consp (rest expr))
           (consp (second expr))
           (eq (first (second expr)) 'lambda)
           (null (cddr expr)))
    (let ((name (if (symbolp name)
                  name
                  (intern name package))))
      ;Avoid trouble when name is a lisp special form like if or lambda.
      (when (special-form-p name)
        (setq name (gensym name)))
      `(flet ((,name ,@(rest (second expr))))
         #',name))
    expr))


; Intern n symbols in the current package with names <prefix>0, <prefix>1, ...,
; <prefix>n-1, where <prefix> is the value of the prefix string.
; Return a list of these n symbols concatenated to the front of rest.
(defun intern-n-vars-with-prefix (prefix n rest)
  (if (zerop n)
    rest
    (intern-n-vars-with-prefix prefix (1- n) (cons (intern (format nil "~A~D" prefix n)) rest))))


; Make a new function with the given name.  The function takes n-args arguments and applies them to the
; function whose source code is in expr.  Return the source code for the function.
(defun gen-defun (expr name n-args)
  (when (special-form-p name)
    (error "Can't call make-defun on ~S" name))
  (if (and (consp expr)
           (eq (first expr) 'function)
           (consp (rest expr))
           (second expr)
           (null (cddr expr))
           (let ((stripped-expr (second expr)))
             (and (consp stripped-expr)
                  (eq (first stripped-expr) 'lambda)
                  (listp (second stripped-expr))
                  (cddr stripped-expr)
                  (every #'(lambda (arg)
                             (and (identifier? arg)
                                  (not (eql (first-symbol-char arg) #\&))))
                         (second stripped-expr)))))
    (let* ((stripped-expr (second expr))
           (function-args (second stripped-expr))
           (function-body (cddr stripped-expr)))
      (assert-true (= (length function-args) n-args))
      (list* 'defun name function-args function-body))
    (let ((args (intern-n-vars-with-prefix "_" n-args nil)))
      (list 'defun name args (apply #'gen-apply expr args)))))


; If code has the form (function <expr>), return <expr>; otherwise, return nil.
(defun simple-strip-function (code)
  (when (and (consp code)
             (eq (first code) 'function)
             (consp (rest code))
             (second code)
             (null (cddr code)))
    (assert-non-null (second code))))


; Strip the (function ...) covering from expr, leaving only a plain lambda expression.
(defun strip-function (expr name n-args)
  (when (special-form-p name)
    (error "Can't call make-defun on ~S" name))
  (if (and (consp expr)
           (eq (first expr) 'function)
           (consp (rest expr))
           (second expr)
           (null (cddr expr))
           (let ((stripped-expr (second expr)))
             (and (consp stripped-expr)
                  (eq (first stripped-expr) 'lambda)
                  (listp (second stripped-expr))
                  (cddr stripped-expr))))
    (second expr)
    (let ((args (intern-n-vars-with-prefix "_" n-args nil)))
      (list 'lambda args (apply #'gen-apply expr args)))))


;;; ------------------------------------------------------------------------------------------------------
;;; LF TOKENS

;;; Each symbol in the LF package is a variant of a terminal that represents that terminal preceded by one
;;; or more line breaks.

(defvar *lf-package* (make-package "LF" :use nil))

(defun make-lf-terminal (terminal)
  (assert-true (not (lf-terminal? terminal)))
  (multiple-value-bind (lf-terminal present) (intern (symbol-name terminal) *lf-package*)
    (unless (eq present :external)
      (export lf-terminal *lf-package*)
      (setf (get lf-terminal :sort-key) (concatenate 'string (symbol-name terminal) " "))
      (setf (get lf-terminal :origin) terminal)
      (setf (get terminal :lf-terminal) lf-terminal))
    lf-terminal))

(defun lf-terminal? (terminal)
  (eq (symbol-package terminal) *lf-package*))


(declaim (inline terminal-lf-terminal lf-terminal-terminal))
(defun terminal-lf-terminal (terminal)
  (get terminal :lf-terminal))
(defun lf-terminal-terminal (lf-terminal)
  (get lf-terminal :origin))


; Ensure that for each transition on a LF: terminal in the grammar there exists a corresponding transition
; on a non-LF: terminal.
(defun ensure-lf-subset (grammar)
  (all-state-transitions
   #'(lambda (state transitions-hash)
       (dolist (transition-pair (state-transitions state))
         (let ((terminal (car transition-pair)))
           (when (lf-terminal? terminal)
             (unless (equal (cdr transition-pair) (gethash (lf-terminal-terminal terminal) transitions-hash))
               (format *error-output* "State ~S: transition on ~S differs from transition on ~S~%"
                       state terminal (lf-terminal-terminal terminal)))))))
   grammar))


; Print a list of transitions on non-LF: terminals that do not have corresponding LF: transitions.
; Return a list of non-LF: terminals which behave identically to the corresponding LF: terminals.
(defun show-non-lf-only-transitions (grammar)
  (let ((invariant-terminalset (make-full-terminalset grammar))
        (terminals-vector (grammar-terminals grammar)))
    (dotimes (n (length terminals-vector))
      (let ((terminal (svref terminals-vector n)))
        (when (lf-terminal? terminal)
          (terminalset-difference-f invariant-terminalset (make-terminalset grammar terminal)))))
    (all-state-transitions
     #'(lambda (state transitions-hash)
         (dolist (transition-pair (state-transitions state))
           (let ((terminal (car transition-pair)))
             (unless (lf-terminal? terminal)
               (let ((lf-terminal (terminal-lf-terminal terminal)))
                 (when lf-terminal
                   (let ((lf-terminal-transition (gethash lf-terminal transitions-hash)))
                     (cond
                      ((null lf-terminal-transition)
                       (terminalset-difference-f invariant-terminalset (make-terminalset grammar terminal))
                       (format *error-output* "State ~S has transition on ~S but not on ~S~%"
                               state terminal lf-terminal))
                      ((not (equal (cdr transition-pair) lf-terminal-transition))
                       (terminalset-difference-f invariant-terminalset (make-terminalset grammar terminal))
                       (format *error-output* "State ~S transition on ~S differs from transition on ~S~%"
                               state terminal lf-terminal))))))))))
     grammar)
    (terminalset-list grammar invariant-terminalset)))


;;; ------------------------------------------------------------------------------------------------------
;;; GRAMMAR-INFO

(defstruct (grammar-info (:constructor make-grammar-info (name grammar &optional lexer))
                         (:copier nil)
                         (:predicate grammar-info?))
  (name nil :type symbol :read-only t)               ;The name of this grammar
  (grammar nil :type grammar :read-only t)           ;This grammar
  (lexer nil :type (or null lexer) :read-only t))    ;This grammar's lexer if this is a lexer grammar; nil if not


; Return the charclass that defines the given lexer nonterminal or nil if none.
(defun grammar-info-charclass (grammar-info nonterminal)
  (let ((lexer (grammar-info-lexer grammar-info)))
    (and lexer (lexer-charclass lexer nonterminal))))


; Return the charclass or partition that defines the given lexer nonterminal or nil if none.
(defun grammar-info-charclass-or-partition (grammar-info nonterminal)
  (let ((lexer (grammar-info-lexer grammar-info)))
    (and lexer (or (lexer-charclass lexer nonterminal)
                   (gethash nonterminal (lexer-partitions lexer))))))


;;; ------------------------------------------------------------------------------------------------------
;;; WORLDS

(defstruct (world (:constructor allocate-world)
                  (:copier nil)
                  (:predicate world?))
  (conditionals nil :type list)                      ;Assoc list of (conditional . highlight), where highlight can be a style keyword, nil (no style), or 'delete
  (package nil :type package)                        ;The package in which this world's identifiers are interned
  (next-type-serial-number 0 :type integer)          ;Serial number to be used for the next type defined
  (types-reverse nil :type (or null hash-table))     ;Hash table of (kind tag parameters) -> type; nil if invalid
  (false-tag nil :type (or null tag))                ;Tag used for false
  (true-tag nil :type (or null tag))                 ;Tag used for true
  (bottom-type nil :type (or null type))             ;Subtype of all types used for nonterminating computations
  (void-type nil :type (or null type))               ;Type used for placeholders
  (false-type nil :type (or null type))              ;Type used for false
  (true-type nil :type (or null type))               ;Type used for true
  (boolean-type nil :type (or null type))            ;Type used for booleans
  (integer-type nil :type (or null type))            ;Type used for integers
  (rational-type nil :type (or null type))           ;Type used for rational numbers
  (finite64-type nil :type (or null type))           ;Type used for nonzero finite double-precision floating-point numbers
  (character-type nil :type (or null type))          ;Type used for characters
  (string-type nil :type (or null type))             ;Type used for strings (vectors of characters)
  (denormalized-false-type nil :type (or null type)) ;Type (denormalized-tag false)
  (denormalized-true-type nil :type (or null type))  ;Type (denormalized-tag true)
  (boxed-boolean-type nil :type (or null type))      ;Union type (union (tag true) (tag false))
  (grammar-infos nil :type list)                     ;List of grammar-info
  (commands-source nil :type list))                  ;List of source code of all commands applied to this world


; Return the name of the world.
(defun world-name (world)
  (package-name (world-package world)))


; Return a symbol in the given package whose value is that package's world structure.
(defun world-access-symbol (package)
  (find-symbol "*WORLD*" package))


; Return the world that created the given package.
(declaim (inline package-world))
(defun package-world (package)
  (symbol-value (world-access-symbol package)))


; Return the world that contains the given symbol.
(defun symbol-world (symbol)
  (package-world (symbol-package symbol)))


; Delete the world and its package.
(defun delete-world (world)
  (let ((package (world-package world)))
    (when package
      (delete-package package)))
  (setf (world-package world) nil))


; Create a world using a package with the given name.
; If the package is already used for another world, its contents
; are erased and the other world deleted.
(defun make-world (name)
  (assert-type name string)
  (let ((p (find-package name)))
    (when p
      (let* ((access-symbol (world-access-symbol p))
             (p-world (and (boundp access-symbol) (symbol-value access-symbol))))
        (unless p-world
          (error "Package ~A already in use" name))
        (assert-true (eq (world-package p-world) p))
        (delete-world p-world))))
  (let* ((p (make-package name :use nil))
         (world (allocate-world
                 :package p
                 :types-reverse (make-hash-table :test #'equal)))
         (access-symbol (intern "*WORLD*" p)))
    (set access-symbol world)
    (export access-symbol p)
    world))


; Intern s (which should be a symbol or a string) in this world's
; package and return the resulting symbol.
(defun world-intern (world s)
  (intern (string s) (world-package world)))


; Same as world-intern except that return nil if s is not already interned.
(defun world-find-symbol (world s)
  (find-symbol (string s) (world-package world)))


; Export symbol in its package, which must belong to some world.
(defun export-symbol (symbol)
  (assert-true (symbol-in-any-world symbol))
  (export symbol (symbol-package symbol)))


; Call f on each external symbol defined in the world's package.
(declaim (inline each-world-external-symbol))
(defun each-world-external-symbol (world f)
  (each-package-external-symbol (world-package world) f))


; Call f on each external symbol defined in the world's package that has
; a property with the given name.
; f takes two arguments:
;   the symbol
;   the value of the property
(defun each-world-external-symbol-with-property (world property f)
  (each-world-external-symbol
   world
   #'(lambda (symbol)
       (let ((value (get symbol property *get2-nonce*)))
         (unless (eq value *get2-nonce*)
           (funcall f symbol value))))))


; Return a list of all external symbols defined in the world's package that have
; a property with the given name.
; The list is sorted by symbol names.
(defun all-world-external-symbols-with-property (world property)
  (let ((list nil))
    (each-world-external-symbol
     world
     #'(lambda (symbol)
         (let ((value (get symbol property *get2-nonce*)))
           (unless (eq value *get2-nonce*)
             (push symbol list)))))
    (sort list #'string<)))


; Return true if s is a symbol that is defined in this world's package.
(declaim (inline symbol-in-world))
(defun symbol-in-world (world s)
  (and (symbolp s) (eq (symbol-package s) (world-package world))))


; Return true if s is a symbol that is defined in any world's package.
(defun symbol-in-any-world (s)
  (and (symbolp s)
       (let* ((package (symbol-package s))
              (access-symbol (world-access-symbol package)))
         (and (boundp access-symbol) (typep (symbol-value access-symbol) 'world)))))


; Return a list of grammars in the world
(defun world-grammars (world)
  (mapcar #'grammar-info-grammar (world-grammar-infos world)))


; Return the grammar-info with the given name in the world
(defun world-grammar-info (world name)
  (find name (world-grammar-infos world) :key #'grammar-info-name))


; Return the grammar with the given name in the world
(defun world-grammar (world name)
  (let ((grammar-info (world-grammar-info world name)))
    (assert-non-null
     (and grammar-info (grammar-info-grammar grammar-info)))))


; Return the lexer with the given name in the world
(defun world-lexer (world name)
  (let ((grammar-info (world-grammar-info world name)))
    (assert-non-null
     (and grammar-info (grammar-info-lexer grammar-info)))))


; Return a list of highlights allowed in this world.
(defun world-highlights (world)
  (let ((highlights nil))
    (dolist (c (world-conditionals world))
      (let ((highlight (cdr c)))
        (unless (or (null highlight) (eq highlight 'delete))
          (pushnew highlight highlights))))
    (nreverse highlights)))


; Return the highlight to which the given conditional maps.
; Return 'delete if the conditional should be omitted.
(defun resolve-conditional (world conditional)
  (let ((h (assoc conditional (world-conditionals world))))
    (if h
      (cdr h)
      (error "Bad conditional ~S" conditional))))


;;; ------------------------------------------------------------------------------------------------------
;;; SYMBOLS

;;; The following properties are attached to exported symbols in the world:
;;;
;;;   :preprocess    preprocessor function ((preprocessor-state id . form-arg-list) -> form-list re-preprocess) if this identifier
;;;                  is a preprocessor command like 'grammar, 'lexer, or 'production
;;;
;;;   :command       expression code generation function ((world grammar-info-var . form-arg-list) -> void) if this identifier
;;;                  is a command like 'deftype or 'define
;;;   :statement     expression code generation function ((world type-env rest last id . form-arg-list) -> codes, live, annotated-stmts)
;;;                  if this identifier is a statement like 'if or 'catch;
;;;                     codes is a list of generated statements, live is true if the statement can fall through, and
;;;                     annotated-stmts is a list of generated annotated statements
;;;   :special-form  expression code generation function ((world type-env id . form-arg-list) -> code, type, annotated-expr)
;;;                  if this identifier is a special form like 'tag or 'in
;;;
;;;   :primitive     primitive structure if this identifier is a primitive
;;;
;;;   :type-constructor  expression code generation function ((world allow-forward-references . form-arg-list) -> type) if this
;;;                  identifier is a type constructor like '->, 'vector, 'set, 'tag, or 'union
;;;   :deftype       type if this identifier is a type; nil if this identifier is a forward-referenced type
;;;
;;;   <value>        value of this identifier if it is a variable of type other than ->
;;;   <function>     value of this identifier if it is a variable of type ->
;;;   :value-expr    unparsed expression defining the value of this identifier if it is a variable
;;;   :type          type of this identifier if it is a variable
;;;   :type-expr     unparsed expression defining the type of this identifier if it is a variable
;;;   :tag           tag structure if this identifier is a tag
;;;   :tag=          a two-argument function that takes two values with this tag and compares them
;;;
;;;   :action        list of (grammar-info . grammar-symbol) that declare this action if this identifier is an action name
;;;
;;;   :depict-command           depictor function ((markup-stream world depict-env . form-arg-list) -> void)
;;;   :depict-statement         depictor function ((markup-stream world . form-annotated-arg-list) -> void)
;;;   :depict-special-form      depictor function ((markup-stream world level . form-annotated-arg-list) -> void)
;;;   :depict-type-constructor  depictor function ((markup-stream world level . form-arg-list) -> void)
;;;


; Return the preprocessor action associated with the given symbol or nil if none.
; This macro is appropriate for use with setf.
(defmacro symbol-preprocessor-function (symbol)
  `(get ,symbol :preprocess))


; Return the primitive definition associated with the given symbol or nil if none.
; This macro is appropriate for use with setf.
(defmacro symbol-primitive (symbol)
  `(get ,symbol :primitive))


; Return the tag definition associated with the given symbol or nil if none.
; This macro is appropriate for use with setf.
(defmacro symbol-tag (symbol)
  `(get ,symbol :tag))


; Call f on each tag definition in the world.
; f takes two arguments:
;   the name
;   the tag structure
(defun each-tag-definition (world f)
  (each-world-external-symbol-with-property world :tag f))


; Return a sorted list of the names of all tag definitions in the world.
(defun world-tag-definitions (world)
  (all-world-external-symbols-with-property world :tag))


; Return the type definition associated with the given symbol.
; Return nil if the symbol is a forward-referenced type.
; If the symbol has no type definition at all, return default
; (or nil if not specified).
; This macro is appropriate for use with setf.
(defmacro symbol-type-definition (symbol &optional default)
  `(get ,symbol :deftype ,@(and default (list default))))


; Return true if this symbol's symbol-type-definition is user-defined.
; This macro is appropriate for use with setf.
(defmacro symbol-type-user-defined (symbol)
  `(get ,symbol 'type-user-defined))


; Call f on each type definition, including forward-referenced types, in the world.
; f takes two arguments:
;   the symbol
;   the type (nil if forward-referenced)
(defun each-type-definition (world f)
  (each-world-external-symbol-with-property world :deftype f))


; Return a sorted list of the names of all type definitions, including
; forward-referenced types, in the world.
(defun world-type-definitions (world)
  (all-world-external-symbols-with-property world :deftype))


; Return the type of the variable associated with the given symbol or nil if none.
; This macro is appropriate for use with setf.
(defmacro symbol-type (symbol)
  `(get ,symbol :type))


; Return a list of (grammar-info . grammar-symbol) pairs that each indicate
; a grammar and a grammar-symbol in that grammar that has an action named by the given symbol.
; This macro is appropriate for use with setf.
(defmacro symbol-action (symbol)
  `(get ,symbol :action))


; Return an unused name for a new function in the world.  The given string is a suggested name.
; The returned value is a symbol.
(defun unique-function-name (world string)
  (let ((f (world-intern world string)))
    (if (fboundp f)
      (gentemp string (world-package world))
      f)))


;;; ------------------------------------------------------------------------------------------------------
;;; TAGS

(defstruct (field (:type list) (:constructor make-field (label type mutable)))
  label                     ;This field's name (not interned in the world)
  type                      ;This field's type 
  mutable)                  ;True if this fields is mutable


(defstruct (tag (:constructor make-tag (name keyword mutable fields =-name link)) (:predicate tag?))
  (name nil :type symbol :read-only t)               ;This tag's world-interned name
  (keyword nil :type (or null keyword) :read-only t) ;This tag's keyword (non-null only when the tag is immutable and has no fields)
  (mutable nil :type bool :read-only t)              ;True if this tag's equality is based on identity, in which case the tag's values have a hidden serial-number field
  (fields nil :type list :read-only t)               ;List of fields after eval-tags-types or (field-name field-type-expression [:const|:var]) before eval-tags-types
  (=-name nil :type symbol)                          ;Lazily computed name of a function that compares two values of this tag for equality; nil if not known yet
  (link nil :type (or null keyword) :read-only t))   ;:reference if this is a local tag, :external if it's a predefined tag, or nil for no cross-references to this tag

; Return three values:
;   the one-based position of the tag's field corresponding to the given label or nil if the label is not present;
;   the type the field; 
;   true if the field is mutable.
(defun tag-find-field (tag label)
  (do ((fields (tag-fields tag) (cdr fields))
       (n (if (tag-mutable tag) 2 1) (1+ n)))
      ((endp fields) (values nil nil nil))
    (let ((field (car fields)))
      (when (eq label (field-label field))
        (return (values n (field-type field) (field-mutable field)))))))


; Define a new tag.  Signal an error if the name is already used.  Return the tag.
; Do not evaluate the field and type expressions yet; that will be done by eval-tags-types.
(defun add-tag (world name mutable fields link)
  (assert-true (member link '(nil :reference :external)))
  (let ((name (scan-name world name)))
    (when (symbol-tag name)
      (error "Attempt to redefine tag ~A" name))
    (let ((keyword nil)
          (=-name nil))
      (unless (or mutable fields)
        (setq keyword (intern (string name) :keyword)))
      (when (or mutable (null fields))
        (setq =-name 'eq)
        (setf (get name :tag=) #'eq))
      (let ((tag (make-tag name keyword mutable fields =-name link)))
        (setf (symbol-tag name) tag)
        (export-symbol name)
        tag))))


; Evaluate the type expressions in the tag's fields.
(defun eval-tag-types (world tag)
  (do ((fields (tag-fields tag) (cdr fields))
       (labels nil))
      ((endp fields))
    (let ((field (first fields)))
      (unless (and (consp field) (identifier? (first field))
                   (consp (cdr field)) (second field)
                   (member (third field) '(nil :const :var))
                   (null (cdddr field)))
        (error "Bad field ~S" field))
      (let ((label (first field))
            (mutable (eq (third field) :var)))
        (when (member label labels)
          (error "Duplicate label ~S" label))
        (push label labels)
        (when (and mutable (not (tag-mutable tag)))
          (error "Tag ~S is immutable but contains a mutable field ~S" (tag-name tag) label))
        (setf (first fields) (make-field label (scan-type world (second field)) mutable))))))


; Evaluate the type expressions in all of the world's tag's fields.
(defun eval-tags-types (world)
  (each-tag-definition
   world
   #'(lambda (name tag)
       (declare (ignore name))
       (eval-tag-types world tag))))


; Return the tag with the given un-world-interned name.  Signal an error if one wasn't found.
(defun scan-tag (world tag-name)
  (let ((name (world-find-symbol world tag-name)))
    (or (symbol-tag name)
        (error "No tag ~A defined" tag-name))))


; Scan label to produce a label that is present in the given tag.
; Return the label's position, its field type, and a flag indicating whether it is mutable.
(defun scan-label (tag label)
  (multiple-value-bind (position field-type mutable) (tag-find-field tag label)
    (unless position
      (error "Label ~S not present in ~A" label (tag-name tag)))
    (values position field-type mutable)))


; Print the tag nicely on the given stream.
(defun print-tag (tag &optional (stream t))
  (pprint-logical-block (stream (tag-fields tag) :prefix "(" :suffix ")")
    (pprint-exit-if-list-exhausted)
    (loop
      (let ((field (pprint-pop)))
        (pprint-logical-block (stream nil :prefix "(" :suffix ")")
          (write (field-label field) :stream stream)
          (format stream " ~@_")
          (print-type (field-type field) stream)
          (when (field-mutable field)
            (format stream " ~@_t")))
        (pprint-exit-if-list-exhausted)
        (format stream " ~:_")))))


;;; ------------------------------------------------------------------------------------------------------
;;; TYPES

(deftype typekind ()
         '(member     ;tag              ;parameters
           :bottom    ;nil              ;nil
           :void      ;nil              ;nil
           :boolean   ;nil              ;nil
           :integer   ;nil              ;nil
           :rational  ;nil              ;nil
           :finite64  ;nil              ;nil    ;All non-zero finite 64-bit double-precision floating-point numbers
           :character ;nil              ;nil
           :->        ;nil              ;(result-type arg1-type arg2-type ... argn-type)
           :string    ;nil              ;(character)
           :vector    ;nil              ;(element-type)
           :set       ;nil              ;(element-type)
           :tag       ;tag              ;nil
           :denormalized-tag ;tag       ;nil
           :union))   ;nil              ;(type ... type) sorted by ascending serial numbers

;A denormalized-tag is a singleton tag type whose value carries no meaning.
;
;All types are normalized except for those with kind :denormalized-tag and the boxed-boolean union type of tags true and false.
;
;A union type must have:
;  at least two types
;  only types with kinds :integer, :rational, :finite64, :character, :string, or :tag
;  no type that is a duplicate or subtype of another type in the union
;  types sorted by ascending type-serial-number.
;
;Note that types with the above kinds never change their serial-numbers during unite-types, so
;unite-types does not need to worry about unions differing only in the order of their parameters.


(defstruct (type (:constructor allocate-type (serial-number kind tag parameters =-name /=-name)) (:predicate type?))
  (name nil :type symbol)                          ;This type's name; nil if this type is anonymous
  (serial-number nil :type integer)                ;This type's unique serial number
  (kind nil :type typekind :read-only t)           ;This type's kind
  (tag nil :type (or null tag) :read-only t)       ;This type's tag
  (parameters nil :type list :read-only t)         ;List of parameter types (either types or symbols if forward-referenced) describing a compound type
  (=-name nil :type symbol)                        ;Lazily computed name of a function that compares two values of this type for equality; nil if not known yet
  (/=-name nil :type symbol))                      ;Name of a function that complements = or nil if none


(declaim (inline make-->-type))
(defun make-->-type (world argument-types result-type)
  (make-type world :-> nil (cons result-type argument-types) nil nil))

(declaim (inline ->-argument-types))
(defun ->-argument-types (type)
  (assert-true (eq (type-kind type) :->))
  (cdr (type-parameters type)))

(declaim (inline ->-result-type))
(defun ->-result-type (type)
  (assert-true (eq (type-kind type) :->))
  (car (type-parameters type)))


(declaim (inline make-vector-type))
(defun make-vector-type (world element-type)
  (if (eq element-type (world-character-type world))
    (world-string-type world)
    (make-type world :vector nil (list element-type) nil nil)))

(declaim (inline vector-element-type))
(defun vector-element-type (type)
  (assert-true (member (type-kind type) '(:vector :string)))
  (car (type-parameters type)))


(declaim (inline make-set-type))
(defun make-set-type (world element-type)
  (make-type world :set nil (list element-type) 'intset= nil))

(declaim (inline set-element-type))
(defun set-element-type (type)
  (assert-true (eq (type-kind type) :set))
  (car (type-parameters type)))


(declaim (inline make-tag-type))
(defun make-tag-type (world tag)
  (make-type world :tag tag nil (tag-=-name tag) nil))


(declaim (inline always-true))
(defun always-true (a b)
  (declare (ignore a b))
  t)

(declaim (inline always-false))
(defun always-false (a b)
  (declare (ignore a b))
  nil)

(declaim (inline make-denormalized-tag-type))
(defun make-denormalized-tag-type (world tag)
  (assert-true (tag-keyword tag))
  (make-type world :denormalized-tag tag nil 'always-true 'always-false))


; Return three values:
;   the one-based position of the type's field corresponding to the given label or nil if the label is not present;
;   the type the field; 
;   true if the field is mutable.
(defun type-find-field (type label)
  (tag-find-field (type-tag type) label))


; Equivalent types are guaranteed to be eq to each other.
(declaim (inline type=))
(defun type= (type1 type2)
  (eq type1 type2))


; Convert a value of a union type that includes finite64 into a value of a union type that includes rational.
(defun union-finite64-to-rational (value)
  (if (floatp value)
    (rational value)
    value))


; code is a lisp expression that evaluates to either :true or :false.
; Return a lisp expression that evaluates code and returns either t or nil.
(defun bool-unboxing-code (code)
  (if (constantp code)
    (ecase code
      (:true t)
      (:false nil))
    (list 'eq code :true)))


; code is a lisp expression that evaluates to either non-nil or nil.
; Return a lisp expression that evaluates code and returns either :true or :false.
(defun bool-boxing-code (code)
  (if (constantp code)
    (ecase code
      ((t) :true)
      ((nil) :false))
    (list 'if code :true :false)))


; code is a lisp expression that evaluates to a value of type type.
; If type is the same or more specific (i.e. a subtype) than supertype, return code that evaluates code
; and returns its value coerced to supertype.
; Signal an error if type is not a subtype of supertype.  expr contains the source code that generated code
; and is used for error reporting only.
;
; Coercions from :denormalized-tag types are not implemented, but they should not be necessary in practice.
(defun widening-coercion-code (world supertype type code expr)
  (if (type= type supertype)
    code
    (flet ((type-mismatch ()
             (error "Expected type ~A for ~:W but got type ~A"
                    (print-type-to-string supertype)
                    expr
                    (print-type-to-string type))))
      (let ((kind (type-kind type)))
        (if (eq kind :bottom)
          code
          (case (type-kind supertype)
            (:boolean
             (if (or (type= type (world-false-type world))
                     (type= type (world-true-type world))
                     (type= type (world-boxed-boolean-type world)))
               (bool-unboxing-code code)
               (type-mismatch)))
            (:rational
             (case kind
               (:integer code)
               (:finite64 (list 'rational code))
               (t (type-mismatch))))
            (:union
             (let ((supertype-types (type-parameters supertype)))
               (case kind
                 (:boolean
                  (if (and (member (world-false-type world) supertype-types) (member (world-true-type world) supertype-types))
                    (bool-boxing-code code)
                    (type-mismatch)))
                 (:integer
                  (if (or (member type supertype-types) (member (world-rational-type world) supertype-types))
                    code
                    (type-mismatch)))
                 ((:rational :character :string :tag)
                  (if (member type supertype-types)
                    code
                    (type-mismatch)))
                 (:finite64
                  (cond
                   ((member type supertype-types) code)
                   ((member (world-rational-type world) supertype-types) (list 'rational code))
                   (t (type-mismatch))))
                 (:union
                  (let ((convert-finite64-to-rational nil))
                    (dolist (type-type (type-parameters type))
                      (unless (case (type-kind type-type)
                                (:integer (or (member type-type supertype-types) (member (world-rational-type world) supertype-types)))
                                ((:rational :character :string :tag) (member type-type supertype-types))
                                (:finite64
                                 (or (member type-type supertype-types)
                                     (and (member (world-rational-type world) supertype-types) (setq convert-finite64-to-rational t)))))
                        (type-mismatch)))
                    (if convert-finite64-to-rational
                      (list 'union-finite64-to-rational code)
                      code)))
                 (t (type-mismatch)))))
            (t (type-mismatch))))))))


; Return the list of constituent types that the given type would have if it were a union.
; The result is sorted by ascending serial numbers and contains no duplicates.
(defun type-to-union (world type)
  (ecase (type-kind type)
    (:boolean (type-parameters (world-boxed-boolean-type world)))
    ((:integer :rational :finite64 :character :string :tag) (list type))
    (:denormalized-tag (make-tag-type world (type-tag type)))
    (:union (type-parameters type))))


; Merge the two lists of types sorted by ascending serial numbers.
; The result is also sorted by ascending serial numbers and contains no duplicates.
(defun merge-type-lists (types1 types2)
  (cond
   ((endp types1) types2)
   ((endp types2) types1)
   (t (let ((type1 (first types1))
            (type2 (first types2)))
        (if (type= type1 type2)
          (cons type1 (merge-type-lists (rest types1) (rest types2)))
          (let ((serial-number1 (type-serial-number type1))
                (serial-number2 (type-serial-number type2)))
            (assert-true (/= serial-number1 serial-number2))
            (if (< serial-number1 serial-number2)
              (cons type1 (merge-type-lists (rest types1) types2))
              (cons type2 (merge-type-lists types1 (rest types2))))))))))


; Return true if the list of types is sorted by serial number.
(defun type-list-sorted (types)
  (let ((n (type-serial-number (first types))))
    (dolist (type (rest types) t)
      (let ((n2 (type-serial-number type)))
        (unless (< n n2)
          (return nil))
        (setq n n2)))))


(defun coercable-to-union-kind (kind)
  (member kind '(:boolean :integer :rational :finite64 :character :string :tag :denormalized-tag :union)))


; types is a list of distinct, non-overlapping types appropriate for inclusion in a union and
; sorted by increasing serial numbers.  Return the union type for holding types, reducing it to
; a simpler type as necessary.  If normalize is nil, don't change the representation of the destination type.
(defun reduce-union-type (world types normalize)
  (cond
   ((endp types) (world-bottom-type world))
   ((endp (cdr types)) (car types))
   ((and (endp (cddr types)) (member (world-true-type world) types) (member (world-false-type world) types))
    (if normalize
      (world-boolean-type world)
      (world-boxed-boolean-type world)))
   ((every #'(lambda (type) (eq (type-=-name type) 'eq)) types)
    (make-type world :union nil types 'eq nil))
   ((every #'(lambda (type) (member (type-=-name type) '(eq eql = char=))) types)
    (make-type world :union nil types 'eql nil))
   (t (make-type world :union nil types nil nil))))


; Return the union of type1 and type2.
(defun type-union (world type1 type2)
  (labels
    ((numeric-kind (kind)
       (member kind '(:integer :rational :finite64)))
     (numeric-type (type)
       (numeric-kind (type-kind type))))
    (if (type= type1 type2)
      type1
      (let ((kind1 (type-kind type1))
            (kind2 (type-kind type2)))
        (cond
         ((eq kind1 :bottom) type2)
         ((eq kind2 :bottom) type1)
         ((and (numeric-kind kind1) (numeric-kind kind2)) (world-rational-type world))
         ((and (coercable-to-union-kind kind1) (coercable-to-union-kind kind2))
          (let ((types (merge-type-lists (type-to-union world type1) (type-to-union world type2))))
            (when (> (count-if #'numeric-type types) 1)
              ;Currently the union of any two or more different numeric types is always rational.
              (setq types (merge-type-lists (remove-if #'numeric-type types) (list (world-rational-type world)))))
            (assert-true (type-list-sorted types))
            (reduce-union-type world types t)))
         (t (error "No union of types ~A and ~A" (print-type-to-string type1) (print-type-to-string type2))))))))


; Return the most specific common supertype of the types.
(defun make-union-type (world &rest types)
  (if types
    (reduce #'(lambda (type1 type2) (type-union world type1 type2))
            types)
    (world-bottom-type world)))


; Ensure that subtype is a subtype of type.  subtype must not be the bottom type.
; Return two values:
;    subtype1, a type that is equivalent to subtype but may be denormalized.
;    subtype2, the type containing the instances of type but not subtype.
; Any concrete value of type will have either subtype1 or subtype2 without needing coercion.
; subtype1 and subtype2 may be denormalized in the following cases:
;    type is boolean and subtype is (tag true) or (tag false);
;    type is a union and subtype is boolean.
; Signal an error if there is no subtype2.
(defun type-difference (world type subtype)
  (flet ((type-mismatch ()
           (error "Cannot subtract type ~A from type ~A" (print-type-to-string subtype) (print-type-to-string type))))
    (if (type= type subtype)
      (if (type= subtype (world-bottom-type world))
        (type-mismatch)
        (values type (world-bottom-type world)))
      (case (type-kind type)
        (:boolean
         (cond
          ((or (type= subtype (world-false-type world)) (type= subtype (world-denormalized-false-type world)))
           (values (world-denormalized-false-type world) (world-denormalized-true-type world)))
          ((or (type= subtype (world-true-type world)) (type= subtype (world-denormalized-true-type world)))
           (values (world-denormalized-true-type world) (world-denormalized-false-type world)))
          ((type= subtype (world-boxed-boolean-type world))
           (values type (world-bottom-type world)))
          (t (type-mismatch))))
        (:tag
          (if (and (eq (type-kind subtype) :denormalized-tag) (eq (type-tag type) (type-tag subtype)))
            (values type (world-bottom-type world))
            (type-mismatch)))
        (:denormalized-tag
         (if (and (eq (type-kind subtype) :tag) (eq (type-tag type) (type-tag subtype)))
           (values type (world-bottom-type world))
           (type-mismatch)))
        (:union
         (let ((types (type-parameters type)))
           (flet
             ((remove-subtype (subtype)
                (unless (member subtype types)
                  (type-mismatch))
                (setq types (remove subtype types))))
             (case (type-kind subtype)
               (:boolean
                (remove-subtype (world-false-type world))
                (remove-subtype (world-true-type world))
                (setq subtype (world-boxed-boolean-type world)))
               (:union
                (mapc #'remove-subtype (type-parameters subtype)))
               (:denormalized-tag
                (remove-subtype (make-tag-type world (type-tag subtype))))
               (t (remove-subtype subtype)))
             (values subtype (reduce-union-type world types nil)))))
        (t (type-mismatch))))))



; types must be a list of types suitable for inclusion in a :union type's parameters.  Return the following values:
;    a list of integerp, rationalp, floatp, characterp, and/or stringp, depending on whether types include the
;       :integer, :rational, :finite64, :character, and/or :string member kinds;
;    a list of keywords used by non-list tags in the types;
;    a list of tag names used by list tags in the types
(defun analyze-union-types (types)
  (let ((atom-tests nil)
        (keywords nil)
        (list-tag-names nil))
    (dolist (type types)
      (ecase (type-kind type)
        (:integer (push 'integerp atom-tests))
        (:rational (push 'rationalp atom-tests))
        (:finite64 (push 'floatp atom-tests))
        (:character (push 'characterp atom-tests))
        (:string (push 'stringp atom-tests))
        (:tag (let* ((tag (type-tag type))
                     (keyword (tag-keyword tag)))
                (if keyword
                  (push keyword keywords)
                  (push (tag-name tag) list-tag-names))))))
    (values
     (nreverse atom-tests)
     (nreverse keywords)
     (nreverse list-tag-names))))


; code is a lisp expression that evaluates to a value of type type.  subtype is a subtype of type, which
; has already been verified by calling type-difference.
; Return a lisp expression that may evaluate code and returns non-nil if the value is a member of the subtype.
; The expression may evaluate code more than once or not at all.
(defun type-member-test-code (world subtype type code)
  (if (type= type subtype)
    t
    (ecase (type-kind type)
      (:boolean
       (cond
        ((or (type= subtype (world-false-type world)) (type= subtype (world-denormalized-false-type world)))
         (list 'not code))
        ((or (type= subtype (world-true-type world)) (type= subtype (world-denormalized-true-type world)))
         code)
        (t (error "Bad type-member-test-code"))))
      ((:tag :denormalized-tag) t)
      (:union
       (multiple-value-bind (type-atom-tests type-keywords type-list-tag-names) (analyze-union-types (type-parameters type))
         (multiple-value-bind (subtype-atom-tests subtype-keywords subtype-list-tag-names)
                              (case (type-kind subtype)
                                (:boolean (values nil (list :false :true) nil))
                                (:union (analyze-union-types (type-parameters subtype)))
                                (:denormalized-tag (analyze-union-types (list (make-tag-type world (type-tag subtype)))))
                                (t (analyze-union-types (list subtype))))
           (assert-true (and (subsetp subtype-atom-tests type-atom-tests)
                             (subsetp subtype-keywords type-keywords)
                             (subsetp subtype-list-tag-names type-list-tag-names)))
           (gen-poly-op 'or nil
                        (nconc 
                         (mapcar #'(lambda (atom-test) (list atom-test code)) subtype-atom-tests)
                         (and subtype-keywords (list (gen-member-test code subtype-keywords)))
                         (and subtype-list-tag-names
                              (list (gen-poly-op 'and t
                                                 (nconc
                                                  (and (or type-atom-tests type-keywords) (list (list 'consp code)))
                                                  (list (gen-member-test (list 'car code) subtype-list-tag-names))))))))))))))



; Return true if type1's serial-number is less than type2's serial-number;
; however, unnamed types' serial numbers are considered to be positive infinity.
(defun type-named-serial-number-< (type1 type2)
  (let ((name1 (if (type-name type1) 0 1))
        (name2 (if (type-name type2) 0 1)))
    (or (< name1 name2)
        (and (= name1 name2)
             (< (type-serial-number type1) (type-serial-number type2))))))


; Print the type nicely on the given stream.  If expand1 is true then print
; the type's top level even if it has a name.  In all other cases expand
; anonymous types but abbreviate named types by their names.
(defun print-type (type &optional (stream t) expand1)
  (if (and (type-name type) (not expand1))
    (write-string (symbol-name (type-name type)) stream)
    (case (type-kind type)
      (:bottom (write-string "bottom" stream))
      (:void (write-string "void" stream))
      (:boolean (write-string "boolean" stream))
      (:integer (write-string "integer" stream))
      (:rational (write-string "rational" stream))
      (:finite64 (write-string "finite64" stream))
      (:character (write-string "character" stream))
      (:-> (pprint-logical-block (stream nil :prefix "(" :suffix ")")
             (format stream "-> ~@_")
             (pprint-indent :current 0 stream)
             (pprint-logical-block (stream (->-argument-types type) :prefix "(" :suffix ")")
               (pprint-exit-if-list-exhausted)
               (loop
                 (print-type (pprint-pop) stream)
                 (pprint-exit-if-list-exhausted)
                 (format stream " ~:_")))
             (format stream " ~_")
             (print-type (->-result-type type) stream)))
      (:string (write-string "string" stream))
      (:vector (pprint-logical-block (stream nil :prefix "(" :suffix ")")
                 (format stream "vector ~@_")
                 (print-type (vector-element-type type) stream)))
      (:set (pprint-logical-block (stream nil :prefix "(" :suffix ")")
              (format stream "set ~@_")
              (print-type (set-element-type type) stream)))
      (:tag (let ((tag (type-tag type)))
              (pprint-logical-block (stream nil :prefix "(" :suffix ")")
                (format stream "tag ~@_~A" (tag-name tag)))))
      (:union (pprint-logical-block (stream (type-parameters type) :prefix "(" :suffix ")")
                (write-string "union" stream)
                (pprint-exit-if-list-exhausted)
                (format stream " ~@_")
                (pprint-indent :current 0 stream)
                (loop
                  (print-type (pprint-pop) stream)
                  (pprint-exit-if-list-exhausted)
                  (format stream " ~:_"))))
      (t (error "Bad typekind ~S" (type-kind type))))))


; Same as print-type except that accumulates the output in a string
; and returns that string.
(defun print-type-to-string (type &optional expand1)
  (with-output-to-string (stream)
    (print-type type stream expand1)))


(defmethod print-object ((type type) stream)
  (print-unreadable-object (type stream)
    (format stream "type~D ~@_" (type-serial-number type))
    (let ((name (type-name type)))
      (when name
        (format stream "~A = ~@_" name)))
    (print-type type stream t)))


; Create or reuse a type with the given kind, tag, and parameters.
; A type is reused if one already exists with equal kind, tag, and parameters.
; Return the type.
(defun make-type (world kind tag parameters =-name /=-name)
  (let ((reverse-key (list kind tag parameters)))
    (or (gethash reverse-key (world-types-reverse world))
        (let ((type (allocate-type (world-next-type-serial-number world) kind tag parameters =-name /=-name)))
          (incf (world-next-type-serial-number world))
          (setf (gethash reverse-key (world-types-reverse world)) type)))))


; Provide a new symbol for the type.  A type can have zero or more names.
; Signal an error if the name is already used.
; user-defined is true if this is a user-defined type rather than a predefined type.
(defun add-type-name (world type symbol user-defined)
  (assert-true (symbol-in-world world symbol))
  (when (symbol-type-definition symbol)
    (error "Attempt to redefine type ~A" symbol))
  ;If the old type was anonymous, give it this name.
  (unless (type-name type)
    (setf (type-name type) symbol))
  (setf (symbol-type-definition symbol) type)
  (when user-defined
    (setf (symbol-type-user-defined symbol) t))
  (export-symbol symbol))


; Return an existing type with the given symbol, which must be interned in a world's package.
; Signal an error if there isn't an existing type.  If allow-forward-references is true and
; symbol is an undefined type identifier, allow it, create a forward-referenced type, and return symbol.
(defun get-type (symbol &optional allow-forward-references)
  (or (symbol-type-definition symbol)
      (if allow-forward-references
        (progn
          (setf (symbol-type-definition symbol) nil)
          symbol)
        (error "Undefined type ~A" symbol))))


; Scan a type-expr to produce a type.  Return that type.
; If allow-forward-references is true and type-expr is an undefined type identifier,
; allow it, create a forward-referenced type in the world, and return type-expr unchanged.
; If allow-forward-references is true, also allow undefined type
; identifiers deeper within type-expr (anywhere except at its top level).
; If type-expr is already a type, return it unchanged.
(defun scan-type (world type-expr &optional allow-forward-references)
  (cond
   ((identifier? type-expr)
    (get-type (world-intern world type-expr) allow-forward-references))
   ((type? type-expr)
    type-expr)
   (t (let ((type-constructor (and (consp type-expr)
                                   (symbolp (first type-expr))
                                   (get (world-find-symbol world (first type-expr)) :type-constructor))))
        (if type-constructor
          (apply type-constructor world allow-forward-references (rest type-expr))
          (error "Bad type ~S" type-expr))))))


; Same as scan-type except that ensure that the type has the expected kind.
; Return the type.
(defun scan-kinded-type (world type-expr expected-type-kind)
  (let ((type (scan-type world type-expr)))
    (unless (eq (type-kind type) expected-type-kind)
      (error "Expected ~(~A~) but got ~A" expected-type-kind (print-type-to-string type)))
    type))


; (-> (<arg-type1> ... <arg-typen>) <result-type>)
(defun scan--> (world allow-forward-references arg-type-exprs result-type-expr)
  (unless (listp arg-type-exprs)
    (error "Bad -> argument type list ~S" arg-type-exprs))
  (make-->-type world
                (mapcar #'(lambda (te) (scan-type world te allow-forward-references)) arg-type-exprs)
                (scan-type world result-type-expr allow-forward-references)))


; (vector <element-type>)
(defun scan-vector (world allow-forward-references element-type)
  (make-vector-type world (scan-type world element-type allow-forward-references)))


; (set <element-type>)
(defun scan-set (world allow-forward-references element-type)
  (make-set-type world (scan-type world element-type allow-forward-references)))


; (tag <tag> ... <tag>)
(defun scan-tag-type (world allow-forward-references tag-name &rest tag-names)
  (if tag-names
    (apply #'make-union-type world (mapcar #'(lambda (tag-name)
                                               (scan-tag-type world allow-forward-references tag-name))
                                           (cons tag-name tag-names)))
    (make-tag-type world (scan-tag world tag-name))))


; (union <type1> ... <typen>)
(defun scan-union (world allow-forward-references &rest type-exprs)
  (apply #'make-union-type world (mapcar #'(lambda (type-expr)
                                             (scan-type world type-expr allow-forward-references))
                                         type-exprs)))


; Resolve all forward type references to refer to their target types.
; Signal an error if any unresolved type references remain.
; Only types reachable from some type name are affected.  It is the caller's
; responsibility to make sure that these are the only types that exist.
; Return a list of all type structures encountered.
(defun resolve-forward-types (world)
  (setf (world-types-reverse world) nil)
  (let ((visited-types (make-hash-table :test #'eq)))
    (labels
      ((resolve-in-type (type)
         (unless (gethash type visited-types)
           (setf (gethash type visited-types) t)
           (do ((parameter-types (type-parameters type) (cdr parameter-types)))
               ((endp parameter-types))
             (let ((parameter-type (car parameter-types)))
               (unless (typep parameter-type 'type)
                 (setq parameter-type (get-type parameter-type))
                 (setf (car parameter-types) parameter-type))
               (resolve-in-type parameter-type))))))
      (each-type-definition
       world
       #'(lambda (symbol type)
           (unless type
             (error "Undefined type ~A" symbol))
           (resolve-in-type type))))
    (hash-table-keys visited-types)))


; Recompute the types-reverse hash table from the types in the types hash table and their constituents.
(defun recompute-type-caches (world)
  (let ((types-reverse (make-hash-table :test #'equal)))
    (labels
      ((visit-type (type)
         (let ((reverse-key (list (type-kind type) (type-tag type) (type-parameters type))))
           (assert-true (eq (gethash reverse-key types-reverse type) type))
           (unless (gethash reverse-key types-reverse)
             (setf (gethash reverse-key types-reverse) type)
             (mapc #'visit-type (type-parameters type))))))
      (visit-type (world-denormalized-false-type world))
      (visit-type (world-denormalized-true-type world))
      (visit-type (world-boxed-boolean-type world))
      (each-type-definition
       world
       #'(lambda (symbol type)
           (declare (ignore symbol))
           (visit-type type))))
    (setf (world-types-reverse world) types-reverse)))



; Make all equivalent types be eq.  Only types reachable from some type name
; are affected, and names may be redirected to different type structures than
; the ones to which they currently point.  It is the caller's responsibility
; to make sure that there are no current outstanding references to types other
; than via type names (except for types for which it can be guaranteed that
; their type structures are defined only once; this applies to types such as
; integer and character but not (vector integer)).
;
; This function calls resolve-forward-types before making equivalent types be eq
; and recompute-type-caches just before returning.
;
; This function works by initially assuming that all types with the same kind
; and tag are the same type and then iterately determining which ones must be
; different because they contain different parameter types.
(defun unite-types (world)
  (let* ((types (resolve-forward-types world))
         (n-types (length types)))
    (labels
      ((gen-cliques-1 (get-key)
         (let ((types-to-cliques (make-hash-table :test #'eq :size n-types))
               (keys-to-cliques (make-hash-table :test #'equal))
               (n-cliques 0))
           (dolist (type types)
             (let* ((key (funcall get-key type))
                    (clique (gethash key keys-to-cliques)))
               (unless clique
                 (setq clique n-cliques)
                 (incf n-cliques)
                 (setf (gethash key keys-to-cliques) clique))
               (setf (gethash type types-to-cliques) clique)))
           (values n-cliques types-to-cliques)))
       
       (gen-cliques (n-old-cliques types-to-old-cliques)
         (labels
           ((get-old-clique (type)
              (assert-non-null (gethash type types-to-old-cliques)))
            (get-type-key (type)
              (cons (get-old-clique type)
                    (mapcar #'get-old-clique (type-parameters type)))))
           (multiple-value-bind (n-new-cliques types-to-new-cliques) (gen-cliques-1 #'get-type-key)
             (assert-true (>= n-new-cliques n-old-cliques))
             (if (/= n-new-cliques n-old-cliques)
               (gen-cliques n-new-cliques types-to-new-cliques)
               (translate-types n-new-cliques types-to-new-cliques)))))
       
       (translate-types (n-cliques types-to-cliques)
         (let ((clique-representatives (make-array n-cliques :initial-element nil)))
           (maphash #'(lambda (type clique)
                        (let ((representative (svref clique-representatives clique)))
                          (when (or (null representative) (type-named-serial-number-< type representative))
                            (setf (svref clique-representatives clique) type))))
                    types-to-cliques)
           (assert-true (every #'identity clique-representatives))
           (labels
             ((map-type (type)
                (svref clique-representatives (gethash type types-to-cliques))))
             (dolist (type types)
               (do ((parameter-types (type-parameters type) (cdr parameter-types)))
                   ((endp parameter-types))
                 (setf (car parameter-types) (map-type (car parameter-types)))))
             (each-type-definition
              world
              #'(lambda (symbol type)
                  (setf (symbol-type-definition symbol) (map-type type))))))))
      
      (multiple-value-call
       #'gen-cliques
       (gen-cliques-1 #'(lambda (type) (cons (type-kind type) (type-tag type)))))
      (recompute-type-caches world))))


;;; ------------------------------------------------------------------------------------------------------
;;; COMPARISONS


; Return non-nil if the values are equal.  value1 and value2 must both belong to a union type.
(defun union= (value1 value2)
  (or (eql value1 value2)
      (and (consp value1) (consp value2)
           (let ((tag-name1 (car value1))
                 (tag-name2 (car value2)))
             (and (eq tag-name1 tag-name2)
                  (funcall (get tag-name1 :tag=) value1 value2))))))


; Create an equality comparison function for elements of the given :vector type.
; Return the name of the function and also set it in the type.
(defun compute-vector-type-=-name (world type)
  (let ((element-type (vector-element-type type)))
    (case (type-kind element-type)
      ((:integer :rational) (setf (type-=-name type) 'equal))
      (t (let ((=-name (gentemp (format nil "~A_VECTOR_=" (type-name element-type)) (world-package world))))
           (setf (type-=-name type) =-name) ;Must do this now to prevent runaway recursion.
           (quiet-compile =-name `(lambda (a b)
                                    (and (= (length a) (length b))
                                         (every #',(get-type-=-name world element-type) a b))))
           =-name)))))


; Create an equality comparison function for elements of the given :tag type.
; Return the name of the function and also set it in the type, the tag, and the :tag= property of the tag-name.
(defun compute-tag-type-=-name (world type)
  (let ((tag (type-tag type)))
    (assert-true (null (tag-=-name tag)))
    (labels
      ((fields-=-code (fields)
         (assert-true fields)
         (let ((field-=-code (cons (get-type-=-name world (field-type (car fields))) '((car a) (car b)))))
           (if (cdr fields)
             `(and ,field-=-code
                   (let ((a (cdr a))
                         (b (cdr b)))
                     ,(fields-=-code (cdr fields))))
             field-=-code))))
      
      (let* ((name (tag-name tag))
             (=-name (world-intern world (concatenate 'string (string name) "_="))))
        (setf (type-=-name type) =-name) ;Must do this now to prevent runaway recursion.
        (let ((=-code `(lambda (a b)
                         (let ((a (cdr a))
                               (b (cdr b)))
                           ,(fields-=-code (tag-fields tag))))))
          (assert-true (not (fboundp =-name)))
          (quiet-compile =-name =-code)
          (setf (get name :tag=) (symbol-function =-name))
          (setf (tag-=-name tag) =-name))))))


; Return the name of a function that compares two instances of this type and returns non-nil if they are equal.
; Signal an error if there is no such function.
; If the type is a tag, also set the :tag= property of the tag.
(defun get-type-=-name (world type)
  (or (type-=-name type)
      (case (type-kind type)
        (:vector (compute-vector-type-=-name world type))
        (:tag (compute-tag-type-=-name world type))
        (:union
         (setf (type-=-name type) 'union=) ;Must do this now to prevent runaway recursion.
         (dolist (subtype (type-parameters type))
           (get-type-=-name world subtype)) ;Set the :tag= symbol properties.
         'union=)
        (t (error "Can't apply = to instances of type ~S" (print-type-to-string type))))))


; Return the name of a function that compares two instances of this type and returns non-nil if they satisfy the given
; order, which should be one of the symbols =, /=, <, >, <=, >=.
; Signal an error if there is no such function except for /=, in which case return nil.
(defun get-type-order-name (world type order)
  (ecase order
    (= (get-type-=-name world type))
    (/= (type-/=-name type))
    ((< > <= >=)
     (or (cdr (assoc order
                     (case (type-kind type)
                       ((:integer :rational) '((< . <) (> . >) (<= . <=) (>= . >=)))
                       (:character '((< . char<) (> . char>) (<= . char<=) (>= . char>=)))
                       (:string '((< . string<) (> . string>) (<= . string<=) (>= . string>=))))))
         (error "Can't apply ~A to instances of type ~A" order (print-type-to-string type))))))


; Return code to compare code expression a against b using the given order, which should be one of
; the symbols =, /=, <, >, <=, >=..
; Signal an error if this is not possible.
(defun get-type-order-code (world type order a b)
  (flet ((simple-constant? (code)
           (or (keywordp code) (numberp code) (characterp code))))
    (let ((order-name (get-type-order-name world type order)))
      (cond
       ((null order-name)
        (assert-true (eq order '/=))
        (list 'not (get-type-order-code world type '= a b)))
       ((and (eq order-name 'union=) (or (simple-constant? a) (simple-constant? b)))
        ;Optimize union= comparisons against a non-list constant.
        (list 'eql a b))
       (t (list order-name a b))))))



;;; ------------------------------------------------------------------------------------------------------
;;; SPECIALS


(defun checked-callable (f)
  (let ((fun (callable f)))
    (unless fun
      (warn "Undefined function ~S" f))
    fun))


; Add a command or special form definition.  symbol is a symbol that names the
; preprocessor directive, command, or special form.  When a semantic form
;   (id arg1 arg2 ... argn)
; is encountered and id is a symbol with the same name as symbol, the form is
; replaced by the result of calling one of:
;   (expander preprocessor-state id arg1 arg2 ... argn)           if property is :preprocess
;   (expander world grammar-info-var arg1 arg2 ... argn)          if property is :command
;   (expander world type-env rest last id arg1 arg2 ... argn)     if property is :statement
;   (expander world type-env id arg1 arg2 ... argn)               if property is :special-form
;   (expander world allow-forward-references arg1 arg2 ... argn)  if property is :type-constructor
; expander must be a function or a function symbol.
;
; In the case of the statement expander only, rest is a list of the remaining statements in the block;
; the statement expander should recursively expand the statements in rest.
; last is non-nil if this statement+rest's return value would pass through as the return value of the function;
; last allows optimization of lisp code to eliminate extraneous return-from statements.
;
; depictor is used instead of expander when emitting markup for the command or special form.
; depictor is called via:
;   (depictor markup-stream world depict-env arg1 arg2 ... argn)     if property is :command
;   (depictor markup-stream world arg1 arg2 ... argn)                if property is :statement
;   (depictor markup-stream world level arg1 arg2 ... argn)          if property is :special-form
;   (depictor markup-stream world level arg1 arg2 ... argn)          if property is :type-constructor
;
(defun add-special (property symbol expander &optional depictor)
  (let ((emit-property (cdr (assoc property '((:command . :depict-command)
                                              (:statement . :depict-statement)
                                              (:special-form . :depict-special-form)
                                              (:type-constructor . :depict-type-constructor))))))
    (assert-true (or emit-property (not depictor)))
    (assert-type symbol identifier)
    (when *value-asserts*
      (checked-callable expander)
      (when depictor (checked-callable depictor)))
    (when (or (get symbol property) (and emit-property (get symbol emit-property)))
      (error "Attempt to redefine ~A ~A" property symbol))
    (setf (get symbol property) expander)
    (when emit-property
      (if depictor
        (setf (get symbol emit-property) depictor)
        (remprop symbol emit-property)))
    (export-symbol symbol)))


;;; ------------------------------------------------------------------------------------------------------
;;; PRIMITIVES

(defstruct (primitive (:constructor make-primitive (type-expr value-code appearance &key markup1 markup2 level level1 level2))
                      (:predicate primitive?))
  (type nil :type (or null type))          ;Type of this primitive; nil if not computed yet
  (type-expr nil :read-only t)             ;Source type expression that designates the type of this primitive
  (value-code nil :read-only t)            ;Lisp expression that computes the value of this primitive
  (appearance nil :read-only t)            ;One of the possible primitive appearances (see below)
  (markup1 nil :read-only t)               ;Markup (item or list) for this primitive
  (markup2 nil :read-only t)               ;:global primitives:  name to use for an external reference
  ;                                        ;:unary primitives:   markup (item or list) for this primitive's closer
  ;                                        ;:infix primitives:   true if spaces should be put around primitive
  (level nil :read-only t)                 ;Precedence level of markup for this primitive
  (level1 nil :read-only t)                ;Precedence level required for first argument of this primitive
  (level2 nil :read-only t))               ;Precedence level required for second argument of this primitive

;appearance is one of the following:
; :global      The primitive appears as a regular, global function or constant; its markup is in markup1.
;                If this primitive should generate an external reference, markup2 contains the name to use for the reference
; :infix       The primitive is an infix binary primitive; its markup is in markup1; if markup2 is true, put spaces around markup1
; :unary       The primitive is a prefix and/or suffix unary primitive; the prefix is in markup1 and suffix in markup2
; :phantom     The primitive disappears when emitting markup for it


; Call this to declare all primitives when initially constructing a world,
; before types have been constructed.
(defun declare-primitive (symbol type-expr value-code appearance &rest key-args)
  (when (symbol-primitive symbol)
    (error "Attempt to redefine primitive ~A" symbol))
  (setf (symbol-primitive symbol) (apply #'make-primitive type-expr value-code appearance key-args))
  (export-symbol symbol))


; Call this to compute the primitive's type from its type-expr.
(defun define-primitive (world primitive)
  (setf (primitive-type primitive) (scan-type world (primitive-type-expr primitive))))


; If name is an identifier not already used by a special form, command, or primitive,
; return it interened into the world's package.  If not, generate an error.
(defun scan-name (world name)
  (unless (identifier? name)
    (error "~S should be an identifier" name))
  (let ((symbol (world-intern world name)))
    (when (get-properties (symbol-plist symbol) '(:command :statement :special-form :primitive :type-constructor))
      (error "~A is reserved" symbol))
    symbol))


;;; ------------------------------------------------------------------------------------------------------
;;; TYPE ENVIRONMENTS

;;; A type environment is an alist that associates bound variables with their types.
;;; A variable may be bound multiple times; the first binding in the environment list
;;; shadows ones further in the list.
;;; The following kinds of bindings are allowed in a type environment:
;;;
;;;   <type-env-local> (see below)
;;;   Normal local variable
;;;
;;;   <type-env-action> (see below)
;;;   Action variable
;;;
;;;   (:return . type)
;;;   The function's return type
;;;
;;;   (:return-block-name . symbol-or-nil)
;;;   The name of the lisp return-from block to be used for returning from this function or nil if not needed yet.
;;;   This binding's symbol-or-nil is mutated in place as needed.
;;;
;;;   (:lhs-symbol . symbol)
;;;   The lhs nonterminal's symbol if this is a type environment for an action function.
;;;

(defstruct (type-env-local (:type list) (:constructor make-type-env-local (name type mode)))
  name      ;World-interned name of the local variable
  type      ;That variable's type
  mode)     ;:const if the variable is read-only; :var if it's writable; :function if it's bound by flet; :unused if it's defined but shouldn't be used

(defstruct (type-env-action (:type list) (:constructor make-type-env-action (key local-symbol type general-grammar-symbol)))
  key                     ;(action symbol . index)
  ;                       ;   action is a world-interned symbol denoting the action function being called
  ;                       ;   symbol is a terminal or nonterminal's symbol on which the action is called
  ;                       ;   index is the one-based index used to distinguish among identical
  ;                       ;     symbols in the rhs of a production.  The first occurrence of this
  ;                       ;     symbol has index 1, the second has index 2, and so on.
  local-symbol            ;A unique local variable name used to represent the action function's value in the generated lisp code
  type                    ;Type of the action function's value
  general-grammar-symbol) ;The general-grammar-symbol corresponding to the index-th instance of symbol in the production's rhs


(defconstant *null-type-env* nil)
(defconstant *type-env-flags* '(:return :return-block-name :lhs-symbol))


; If symbol is a local variable, return its binding; if not, return nil.
; symbol must already be world-interned.
(declaim (inline type-env-get-local))
(defun type-env-get-local (type-env symbol)
  (assoc symbol type-env :test #'eq))


; If the currently generated function is an action for a rule with at least index
; instances of the given grammar-symbol's symbol on the right-hand side, and if action is
; a legal action for that symbol, return three values:
;   the name to use from the generated lisp code to refer to the result of calling
;     the action on the index-th instance of this symbol;
;   the action result's type;
;   the general-grammar-symbol corresponding to the index-th instance of this symbol in the rhs.
; Otherwise, return nil.
; action must already be world-interned.
(defun type-env-get-action (type-env action symbol index)
  (assoc (list* action symbol index) type-env :test #'equal))


; Nondestructively append the binding to the front of the type-env and return the new type-env.
; If shadow is true, the binding may shadow an existing local variable with the same name.
(defun type-env-add-binding (type-env name type mode &optional shadow)
  (assert-true (and
                (symbolp name)
                (type? type)
                (member mode '(:const :var :function :unused))))
  (unless shadow
    (let ((binding (type-env-get-local type-env name)))
      (when binding
        (error "Local variable ~A:~A shadows an existing local variable ~A:~A"
               name (print-type-to-string type)
               (type-env-local-name binding) (print-type-to-string (type-env-local-type binding))))))
  (cons (make-type-env-local name type mode) type-env))


; Nondestructively shadow the type of the binding of name in type-env and return the new type-env.
(defun type-env-narrow-binding (type-env name type)
  (let ((binding (assert-non-null (type-env-get-local type-env name))))
    (type-env-add-binding type-env name type (type-env-local-mode binding) t)))


; Nondestructively shadow all writable bindings in the type-env by unused bindings and return the new type-env.
; Also create new bindings for the function's return type and return block name.
(defun type-env-init-function (type-env return-type)
  (dolist (binding type-env)
    (let ((name (first binding)))
      (when (and (symbolp name) (not (keywordp name)) (eq (type-env-local-mode binding) :var))
        (let* ((first-binding (type-env-get-local type-env name))
               (first-mode (type-env-local-mode first-binding)))
          (assert-true first-mode)
          (unless (eq first-mode :unused)
            (push (make-type-env-local (type-env-local-name first-binding) (type-env-local-type first-binding) :unused) type-env))))))
  (set-type-env-flag
   (set-type-env-flag type-env :return return-type)
   :return-block-name
   nil))


; Either reuse or generate a name for return-from statements exiting this function.
(defun gen-type-env-return-block-name (type-env)
  (let ((return-block-binding (assert-non-null (assoc :return-block-name type-env))))
    (or (cdr return-block-binding)
        (setf (cdr return-block-binding) (gensym "RETURN")))))


; Return an environment obtained from the type-env by adding a binding of flag to value.
(defun set-type-env-flag (type-env flag value)
  (assert-true (member flag *type-env-flags*))
  (acons flag value type-env))


; Return the value bound to the given flag.
(defun get-type-env-flag (type-env flag)
  (assert-true (member flag *type-env-flags*))
  (cdr (assoc flag type-env)))


;;; ------------------------------------------------------------------------------------------------------
;;; VALUES

;;; A value is one of the following:
;;;   A void value (represented by nil)
;;;   A boolean (nil for false; non-nil for true)
;;;   An integer
;;;   A rational number
;;;   A double-precision floating-point number (or :+infinity, :-infinity, or :nan)
;;;   A character
;;;   A function (represented by a lisp function)
;;;   A string
;;;   A vector (represented by a list)
;;;   A set (represented by an intset of its elements converted to integers)
;;;   A tag (represented by either a keyword or a list (keyword [serial-num] field-value1 ... field-value n));
;;;     serial-num is a unique integer present only on mutable tag instances.


; Return true if the value appears to have the given tag.  This function
; may return false positives (return true when the value doesn't actually
; have the given type) but never false negatives.
; If shallow is true, only test at the top level.
(defun value-has-tag (value tag &optional shallow)
  (labels
    ((check-fields (fields values)
       (if (endp fields)
         (null values)
         (and (consp values)
              (or shallow (value-has-type (car values) (field-type (car fields))))
              (check-fields (cdr fields) (cdr values))))))
    (let ((keyword (tag-keyword tag)))
      (if keyword
        (eq value keyword)
        (and (consp value)
             (eq (car value) (tag-name tag))
             (let ((values (cdr value))
                   (fields (tag-fields tag)))
               (if (tag-mutable tag)
                 (and (consp values) (integerp (car values)) (check-fields fields (cdr values)))
                 (check-fields fields values))))))))


; Return true if the value appears to have the given type.  This function
; may return false positives (return true when the value doesn't actually
; have the given type) but never false negatives.
; If shallow is true, only test at the top level.
(defun value-has-type (value type &optional shallow)
  (case (type-kind type)
    (:bottom nil)
    (:void t)
    (:boolean t)
    (:integer (integerp value))
    (:rational (rationalp value))
    (:finite64 (and (floatp value) (not (zerop value))))
    (:character (characterp value))
    (:-> (functionp value))
    (:string (stringp value))
    (:vector (let ((element-type (vector-element-type type)))
               (labels
                 ((test (value)
                    (or (null value)
                        (and (consp value)
                             (or shallow (value-has-type (car value) element-type))
                             (test (cdr value))))))
                 (test value))))
    (:set (valid-intset? value))
    (:tag (value-has-tag value (type-tag type) shallow))
    (:union (some #'(lambda (subtype) (value-has-type value subtype shallow))
                  (type-parameters type)))
    (t (error "Bad typekind ~S" (type-kind type)))))


; Print the value nicely on the given stream.  type is the value's type.
(defun print-value (value type &optional (stream t))
  (assert-true (value-has-type value type t))
  (case (type-kind type)
    (:void (assert-true (null value))
           (write-string "empty" stream))
    (:boolean (write-string (if value "true" "false") stream))
    ((:integer :rational :character :->) (write value :stream stream))
    (:finite64 (write value :stream stream))
    (:string (prin1 value stream))
    (:vector (let ((element-type (vector-element-type type)))
               (pprint-logical-block (stream value :prefix "(" :suffix ")")
                 (pprint-exit-if-list-exhausted)
                 (loop
                   (print-value (pprint-pop) element-type stream)
                   (pprint-exit-if-list-exhausted)
                   (format stream " ~:_")))))
    (:set (let ((converter (set-out-converter (set-element-type type))))
            (pprint-logical-block (stream value :prefix "{" :suffix "}")
              (pprint-exit-if-list-exhausted)
              (loop
                (let* ((values (pprint-pop))
                       (value1 (car values))
                       (value2 (cdr values)))
                  (if (= value1 value2)
                    (write (funcall converter value1) :stream stream)
                    (write (list (funcall converter value1) (funcall converter value2)) :stream stream))))
              (pprint-exit-if-list-exhausted)
              (format stream " ~:_"))))
    (:tag (let ((tag (type-tag type)))
            (if (tag-keyword tag)
              (write value :stream stream)
              (pprint-logical-block (stream (tag-fields tag) :prefix "[" :suffix "]")
                (write (pop value) :stream stream)
                (when (tag-mutable tag)
                  (format stream " ~:_~D" (pop value)))
                (loop
                  (pprint-exit-if-list-exhausted)
                  (format stream " ~:_")
                  (print-value (pop value) (field-type (pprint-pop)) stream))))))
    (:union (dolist (subtype (type-parameters type)
                             (error "~S is not an instance of ~A" value (print-type-to-string type)))
              (when (value-has-type value subtype t)
                (print-value value subtype stream)
                (return))))
    (t (error "Bad typekind ~S" (type-kind type)))))


; Print a list of values nicely on the given stream.  types is the list of the
; values' types (and should have the same length as the list of values).
; If prefix and/or suffix are non-null, use them as beginning and ending
; delimiters of the printed list.
(defun print-values (values types &optional (stream t) &key prefix suffix)
  (assert-true (= (length values) (length types)))
  (pprint-logical-block (stream values :prefix prefix :suffix suffix)
    (pprint-exit-if-list-exhausted)
    (dolist (type types)
      (print-value (pprint-pop) type stream)
      (pprint-exit-if-list-exhausted)
      (format stream " ~:_"))))


;;; ------------------------------------------------------------------------------------------------------
;;; VALUE EXPRESSIONS

;;; Expressions are annotated to avoid having to duplicate the expression scanning logic when
;;; emitting markup for expressions.  Expression forms are prefixed with an expr-annotation symbol
;;; to indicate their kinds.  These symbols are in their own package to avoid potential confusion
;;; with keywords, variable names, terminals, etc.
;;;
;;; Some special forms are extended to include parsed type information for the benefit of markup logic.
(eval-when (:compile-toplevel :load-toplevel :execute)
  (defpackage "EXPR-ANNOTATION"
    (:use)
    (:export "CONSTANT"        ;(expr-annotation:constant <constant>)
             "PRIMITIVE"       ;(expr-annotation:primitive <interned-id>)
             "TAG"             ;(expr-annotation:tag <tag>)
             "LOCAL"           ;(expr-annotation:local <interned-id>)      ;Local or lexically scoped variable
             "GLOBAL"          ;(expr-annotation:global <interned-id>)     ;Global variable
             "CALL"            ;(expr-annotation:call <function-expr> <arg-expr> ... <arg-expr>)
             "ACTION"          ;(expr-annotation:action <action> <general-grammar-symbol> <optional-index>)
             "BEGIN"           ;(expr-annotation:begin <statement> ... <statement>)
             "SPECIAL-FORM"))) ;(expr-annotation:special-form <interned-form> ...)


; Return true if the annotated-stmt is a statement with the given special-form, which must be a symbol
; but does not have to be interned in the world's package.
(defun special-form-annotated-stmt? (world special-form annotated-stmt)
  (eq (first annotated-stmt) (world-find-symbol world special-form)))


; Return true if the annotated-expr is a special form annotated expression with
; the given special-form.  special-form must be a symbol but does not have to be interned
; in the world's package.
(defun special-form-annotated-expr? (world special-form annotated-expr)
  (and (eq (first annotated-expr) 'expr-annotation:special-form)
       (eq (second annotated-expr) (world-find-symbol world special-form))))


; Return the value of the variable with the given symbol.
; Compute the value if the variable was unbound.
; Use the *busy-variables* list to prevent infinite recursion while computing variable values.
(defmacro fetch-value (symbol)
  `(if (boundp ',symbol)
     (symbol-value ',symbol)
     (compute-variable-value ',symbol)))


; Generate a lisp expression that will compute the value of value-expr.
; type-env is the type environment.  The expression may refer to free variables
; present in the type-env.
; Return three values:
;   The expression's value (a lisp expression)
;   The expression's type
;   The annotated value-expr
(defun scan-value (world type-env value-expr)
  (labels
    ((syntax-error ()
       (error "Syntax error: ~S" value-expr))
     
     ;Scan a function call.  The function has already been scanned into its value and type,
     ;but the arguments are still unprocessed.
     (scan-call (function-value function-type function-annotated-expr arg-exprs)
       (let ((arg-values nil)
             (arg-types nil)
             (arg-annotated-exprs nil))
         (dolist (arg-expr arg-exprs)
           (multiple-value-bind (arg-value arg-type arg-annotated-expr) (scan-value world type-env arg-expr)
             (push arg-value arg-values)
             (push arg-type arg-types)
             (push arg-annotated-expr arg-annotated-exprs)))
         (let ((arg-values (nreverse arg-values))
               (arg-types (nreverse arg-types))
               (arg-annotated-exprs (nreverse arg-annotated-exprs)))
           (handler-bind (((or error warning)
                           #'(lambda (condition)
                               (declare (ignore condition))
                               (format *error-output*
                                       "~@<In ~S: ~_Function of type ~A called with arguments of types~:_~{ ~A~}~:>"
                                       value-expr
                                       (print-type-to-string function-type)
                                       (mapcar #'print-type-to-string arg-types)))))
             (unless (eq (type-kind function-type) :->)
               (error "Non-function called"))
             (let ((parameter-types (->-argument-types function-type)))
               (unless (= (length arg-types) (length parameter-types))
                 (error "Argument count mismatch"))
               (let ((arg-values (mapcar #'(lambda (arg-expr arg-value arg-type parameter-type)
                                             (widening-coercion-code world parameter-type arg-type arg-value arg-expr))
                                         arg-exprs arg-values arg-types parameter-types)))
                 (values (apply #'gen-apply function-value arg-values)
                         (->-result-type function-type)
                         (list* 'expr-annotation:call function-annotated-expr arg-annotated-exprs))))))))
     
     ;Scan an action call
     (scan-action-call (action symbol &optional (index 1 index-supplied))
       (unless (integerp index)
         (error "Production rhs grammar symbol index ~S must be an integer" index))
       (let ((symbol-action (type-env-get-action type-env action symbol index)))
         (unless symbol-action
           (error "Action ~S not found" (list action symbol index)))
         (let ((multiple-symbols (type-env-get-action type-env action symbol 2)))
           (when (and (not index-supplied) multiple-symbols)
             (error "Ambiguous index in action ~S" (list action symbol)))
           (values (type-env-action-local-symbol symbol-action)
                   (type-env-action-type symbol-action)
                   (list* 'expr-annotation:action action (type-env-action-general-grammar-symbol symbol-action)
                          (and (or multiple-symbols
                                   (grammar-symbol-= symbol (assert-non-null (get-type-env-flag type-env :lhs-symbol))))
                               (list index)))))))
     
     ;Scan an interned identifier
     (scan-identifier (symbol)
       (let ((symbol-binding (type-env-get-local type-env symbol)))
         (if symbol-binding
           (ecase (type-env-local-mode symbol-binding)
             ((:const :var)
              (values (type-env-local-name symbol-binding)
                      (type-env-local-type symbol-binding)
                      (list 'expr-annotation:local symbol)))
             (:function
               (values (list 'function (type-env-local-name symbol-binding))
                       (type-env-local-type symbol-binding)
                       (list 'expr-annotation:local symbol)))
             (:unused (error "Unused variable ~A referenced" symbol)))
           (let ((primitive (symbol-primitive symbol)))
             (if primitive
               (values (primitive-value-code primitive) (primitive-type primitive) (list 'expr-annotation:primitive symbol))
               (let ((tag (symbol-tag symbol)))
                 (if (and tag (tag-keyword tag))
                   (values (tag-keyword tag)
                           (make-tag-type world tag)
                           (list 'expr-annotation:tag tag))
                   (let ((type (symbol-type symbol)))
                     (if type
                       (values (if (eq (type-kind type) :->)
                                 (list 'symbol-function (list 'quote symbol))
                                 (list 'fetch-value symbol))
                               type
                               (list 'expr-annotation:global symbol))
                       (syntax-error))))))))))
     
     ;Scan a call or special form
     (scan-cons (first rest)
       (if (identifier? first)
         (let ((symbol (world-intern world first)))
           (let ((handler (get symbol :special-form)))
             (if handler
               (apply handler world type-env symbol rest)
               (if (and (symbol-action symbol)
                        (let ((local (type-env-get-local type-env symbol)))
                          (not (and local (eq (type-kind (type-env-local-type local)) :->)))))
                 (apply #'scan-action-call symbol rest)
                 (multiple-value-call #'scan-call (scan-identifier symbol) rest)))))
         (multiple-value-call #'scan-call (scan-value world type-env first) rest)))
     
     (scan-constant (value-expr type)
       (values value-expr type (list 'expr-annotation:constant value-expr))))
    
    (assert-three-values
     (cond
      ((consp value-expr) (scan-cons (first value-expr) (rest value-expr)))
      ((identifier? value-expr) (scan-identifier (world-intern world value-expr)))
      ((integerp value-expr) (scan-constant value-expr (world-integer-type world)))
      ((floatp value-expr)
       (if (zerop value-expr)
         (error "Use +zero or -zero instead of 0.0")
         (scan-constant value-expr (world-finite64-type world))))
      ((characterp value-expr) (scan-constant value-expr (world-character-type world)))
      ((stringp value-expr) (scan-constant value-expr (world-string-type world)))
      (t (syntax-error))))))


; Same as scan-value except that ensure that the value has the expected type.
; Return two values:
;   The expression's value (a lisp expression)
;   The annotated value-expr
(defun scan-typed-value (world type-env value-expr expected-type)
  (multiple-value-bind (value type annotated-expr) (scan-value world type-env value-expr)
    (values (widening-coercion-code world expected-type type value value-expr) annotated-expr)))


; Same as scan-value except that ensure that the value has type bottom or void.
; Return three values:
;   The expression's value (a lisp expression)
;   True if value has type void
;   The annotated value-expr
(defun scan-void-value (world type-env value-expr)
  (multiple-value-bind (value type annotated-expr) (scan-value world type-env value-expr)
    (values
     value
     (case (type-kind type)
       (:bottom nil)
       (:void t)
       (t (error "Value ~S:~A should be void" value-expr (print-type-to-string type))))
     annotated-expr)))


; Same as scan-value except that ensure that the value is a vector type.
; Return three values:
;   The expression's value (a lisp expression)
;   The expression's type
;   The annotated value-expr
(defun scan-vector-value (world type-env value-expr)
  (multiple-value-bind (value type annotated-expr) (scan-value world type-env value-expr)
    (unless (member (type-kind type) '(:string :vector))
      (error "Value ~S:~A should be a vector" value-expr (print-type-to-string type)))
    (values value type annotated-expr)))


; Same as scan-value except that ensure that the value is a tag type.
; Return three values:
;   The expression's value (a lisp expression)
;   The expression's type
;   The annotated value-expr
(defun scan-tag-value (world type-env value-expr)
  (multiple-value-bind (value type annotated-expr) (scan-value world type-env value-expr)
    (unless (eq (type-kind type) :tag)
      (error "Value ~S:~A should be a tag" value-expr (print-type-to-string type)))
    (values value type annotated-expr)))


; Return the code for computing value-expr, which will be assigned to the symbol.  Check that the
; value has the given type.
(defun scan-global-value (symbol value-expr type)
  (scan-typed-value (symbol-world symbol) *null-type-env* value-expr type))


; Same as scan-typed-value except that also allow the form (begin . <statements>); in this case
; return can be used to return the expression's value.
; Return two values:
;   The expression's value (a lisp expression)
;   The annotated value-expr
(defun scan-typed-value-or-begin (world type-env value-expr expected-type)
  (if (and (consp value-expr) (eq (first value-expr) 'begin))
    (let* ((result-type (scan-type world expected-type))
           (local-type-env (type-env-init-function type-env result-type)))
      (multiple-value-bind (body-codes body-annotated-stmts) (finish-function-code world local-type-env result-type (cdr value-expr))
        (values (gen-progn body-codes)
                (cons 'expr-annotation:begin body-annotated-stmts))))
    (scan-typed-value world type-env value-expr expected-type)))



; Generate the defun code for the world's variable named by symbol.
; The variable's type must be ->.
(defun compute-variable-function (symbol value-expr type)
  (handler-bind (((or error warning)
                  #'(lambda (condition)
                      (declare (ignore condition))
                      (format *error-output* "~&~@<~2IWhile computing ~A: ~_~:W~:>~%" symbol value-expr))))
    (assert-true (not (or (boundp symbol) (fboundp symbol))))
    (let ((code (strip-function (scan-global-value symbol value-expr type) symbol (length (->-argument-types type)))))
      (when *trace-variables*
        (format *trace-output* "~&~S ::= ~:W~%" symbol code))
      (quiet-compile symbol code))))


(defvar *busy-variables* nil)


; Compute the value of a world's variable named by symbol.  Return the variable's value.
; If the variable already has a computed value, return it unchanged.  The variable's type must not be ->.
; If computing the value requires the values of other variables, compute them as well.
; Use the *busy-variables* list to prevent infinite recursion while computing variable values.
(defun compute-variable-value (symbol)
  (cond
   ((member symbol *busy-variables*) (error "Definition of ~A refers to itself" symbol))
   ((boundp symbol) (symbol-value symbol))
   ((fboundp symbol) (error "compute-variable-value should be called only once on a function"))
   (t (let* ((*busy-variables* (cons symbol *busy-variables*))
             (value-expr (get symbol :value-expr))
             (type (symbol-type symbol)))
        (handler-bind (((or error warning)
                        #'(lambda (condition)
                            (declare (ignore condition))
                            (format *error-output* "~&~@<~2IWhile computing ~A: ~_~:W~:>~%"
                                    symbol value-expr))))
          (assert-true (not (eq (type-kind type) :->)))
          (let ((value-code (scan-global-value symbol value-expr type)))
            (when *trace-variables*
              (format *trace-output* "~&~S := ~:W~%" symbol value-code))
            (set symbol (eval value-code))))))))


;;; ------------------------------------------------------------------------------------------------------
;;; SPECIAL FORMS

;;; Constants

(defun eval-bottom ()
  (error "Reached a BOTTOM expression"))

; (bottom)
; (todo)
; Raises an error.
(defun scan-bottom (world type-env special-form)
  (declare (ignore type-env))
  (values
   '(eval-bottom)
   (world-bottom-type world)
   (list 'expr-annotation:special-form special-form)))


; (hex <integer> [<length>])
; Alternative way of writing the integer in hexadecimal.  length is the minimum number of digits to write.
(defun scan-hex (world type-env special-form n &optional (length 1))
  (declare (ignore type-env))
  (unless (and (integerp n) (integerp length) (>= length 0))
    (error "Bad hex constant ~S [~S]" n length))
  (values
   n
   (world-integer-type world)
   (list 'expr-annotation:special-form special-form n length)))


;;; Expressions


(defun semantic-expt (base exponent)
  (assert-true (and (rationalp base) (integerp exponent)))
  (when (and (zerop base) (not (plusp exponent)))
    (error "0 raised to a nonpositive exponent"))
  (expt base exponent))


; (expt <base> <exponent>)
; The result is rational unless both base and exponent are integer constants and the result is an integer.
(defun scan-expt (world type-env special-form base-expr exponent-expr)
  (multiple-value-bind (base-code base-annotated-expr) (scan-typed-value world type-env base-expr (world-rational-type world))
    (multiple-value-bind (exponent-code exponent-annotated-expr) (scan-typed-value world type-env exponent-expr (world-integer-type world))
      (let ((code (list 'semantic-expt base-code exponent-code))
            (type (world-rational-type world)))
        (when (and (constantp base-code) (constantp exponent-code))
          (setq code (semantic-expt base-code exponent-code))
          (when (integerp code)
            (setq type (world-integer-type world))))
        (values
         code
         type
         (list 'expr-annotation:special-form special-form base-annotated-expr exponent-annotated-expr))))))


; Return the depict name for one of the comparison symbols =, /=, <, >, <=, >=.
(defun comparison-name (order)
  (cdr (assoc order '((= . "=") (/= . :not-equal) (< . "<") (> . ">") (<= . :less-or-equal) (>= . :greater-or-equal)))))


; Both expr1 and expr2 are coerced to the given type and then compared using the given order.
; The result is a boolean.  order-name should be suitable for depict.
(defun scan-comparison (world type-env special-form order expr1 expr2 type-expr)
  (let ((type (scan-type world type-expr)))
    (multiple-value-bind (code1 annotated-expr1) (scan-typed-value world type-env expr1 type)
      (multiple-value-bind (code2 annotated-expr2) (scan-typed-value world type-env expr2 type)
        (values
         (get-type-order-code world type order code1 code2)
         (world-boolean-type world)
         (list 'expr-annotation:special-form special-form (comparison-name order) annotated-expr1 annotated-expr2))))))


; (= <expr1> <expr2> [<type>])
(defun scan-= (world type-env special-form expr1 expr2 &optional (type-expr 'integer))
  (scan-comparison world type-env special-form '= expr1 expr2 type-expr))

; (/= <expr1> <expr2> [<type>])
(defun scan-/= (world type-env special-form expr1 expr2 &optional (type-expr 'integer))
  (scan-comparison world type-env special-form '/= expr1 expr2 type-expr))

; (< <expr1> <expr2> [<type>])
(defun scan-< (world type-env special-form expr1 expr2 &optional (type-expr 'integer))
  (scan-comparison world type-env special-form '< expr1 expr2 type-expr))

; (> <expr1> <expr2> [<type>])
(defun scan-> (world type-env special-form expr1 expr2 &optional (type-expr 'integer))
  (scan-comparison world type-env special-form '> expr1 expr2 type-expr))

; (<= <expr1> <expr2> [<type>])
(defun scan-<= (world type-env special-form expr1 expr2 &optional (type-expr 'integer))
  (scan-comparison world type-env special-form '<= expr1 expr2 type-expr))

; (>= <expr1> <expr2> [<type>])
(defun scan->= (world type-env special-form expr1 expr2 &optional (type-expr 'integer))
  (scan-comparison world type-env special-form '>= expr1 expr2 type-expr))


; (cascade <type> <expr1> <order1> <expr2> <order2> ... <ordern-1> <exprn>)
; Shorthand for (and (<order1> <expr1> <expr2> <type>) (<order1> <expr2> <expr3> <type>) ... (<ordern-1> <exprn-1> <exprn> <type>)),
; where each order must be one of the symbols =, /=, <, >, <=, >=.
; The intermediate expressions are evaluated at most once.
(defun scan-cascade (world type-env special-form type-expr expr1 &rest orders-and-exprs)
  (let ((type (scan-type world type-expr)))
    (labels
      ((cascade (v1 orders-and-exprs)
         (unless (and (consp orders-and-exprs) (consp (cdr orders-and-exprs))
                      (member (first orders-and-exprs) '(= /= < > <= >=)))
           (error "Bad cascade tail: ~S" orders-and-exprs))
         (let* ((order (first orders-and-exprs))
                (order-name (comparison-name order))
                (expr2 (second orders-and-exprs))
                (orders-and-exprs (cddr orders-and-exprs)))
           (multiple-value-bind (code2 annotated-expr2) (scan-typed-value world type-env expr2 type)
             (if orders-and-exprs
               (let ((v2 (gensym "L")))
                 (multiple-value-bind (codes annotations) (cascade v2 orders-and-exprs)
                   (values
                    `(let ((,v2 ,code2))
                       (and ,(get-type-order-code world type order v1 v2) ,codes))
                    (list* order-name annotated-expr2 annotations))))
               (values
                (get-type-order-code world type order v1 code2)
                (list order-name annotated-expr2)))))))
      
      (multiple-value-bind (code1 annotated-expr1) (scan-typed-value world type-env expr1 type)
        (let ((v1 (gensym "L")))
          (multiple-value-bind (codes annotations) (cascade v1 orders-and-exprs)
            (values
             `(let ((,v1 ,code1)) ,codes)
             (world-boolean-type world)
             (list* 'expr-annotation:special-form special-form annotated-expr1 annotations))))))))


; (and <expr> ... <expr>)
; Short-circuiting logical AND.
(defun scan-and (world type-env special-form expr &rest exprs)
  (apply #'scan-and-or-xor world type-env special-form 'and t expr exprs))

; (or <expr> ... <expr>)
; Short-circuiting logical OR.
(defun scan-or (world type-env special-form expr &rest exprs)
  (apply #'scan-and-or-xor world type-env special-form 'or nil expr exprs))

; (xor <expr> ... <expr>)
; Logical XOR.
(defun scan-xor (world type-env special-form expr &rest exprs)
  (apply #'scan-and-or-xor world type-env special-form 'xor nil expr exprs))

(defun scan-and-or-xor (world type-env special-form op identity &rest exprs)
  (multiple-value-map-bind (codes annotated-exprs)
                           #'(lambda (expr)
                               (scan-typed-value world type-env expr (world-boolean-type world)))
                           (exprs)
    (values
     (gen-poly-op op identity codes)
     (world-boolean-type world)
     (list* 'expr-annotation:special-form special-form op annotated-exprs))))


; (begin . <statements>)
; Only allowed at the top level of an action.


(defun finish-function-code (world type-env result-type body-statements)
  (multiple-value-bind (body-codes body-live body-annotated-stmts) (scan-statements world type-env body-statements t t)
    (when (and body-live (not (or (type= result-type (world-void-type world))
                                  (type= result-type (world-bottom-type world)))))
      (error "Execution falls off the end of a function with result type ~A" (print-type-to-string result-type)))
    (let ((return-block-name (get-type-env-flag type-env :return-block-name)))
      (values
       (if return-block-name
         (list (list* 'block return-block-name body-codes))
         body-codes)
       body-annotated-stmts))))


; Scan a local function.
;   arg-binding-exprs should have the form ((<var1> <type1> [:unused]) ... (<varn> <typen> [:unused])).
;   result-type-expr should be a type expression.
;   body-statements contains the function's body statements.
; Return three values:
;   A list of lisp function bindings followed by the code (i.e. '((a b c) (declare (ignore c)) (* a b)));
;   The function's complete type;
;   The annotated body statements.
(defun scan-function-or-lambda (world type-env arg-binding-exprs result-type-expr body-statements)
  (handler-bind (((or error warning)
                  #'(lambda (condition)
                      (declare (ignore condition))
                      (format *error-output* "~&~@<~2IWhile processing lambda ~_~S ~_~S ~_~S:~:>~%"
                              arg-binding-exprs result-type-expr body-statements))))
    (let* ((result-type (scan-type world result-type-expr))
           (local-type-env (type-env-init-function type-env result-type))
           (args nil)
           (arg-types nil)
           (unused-args nil))
      (unless (listp arg-binding-exprs)
        (error "Bad function bindings ~S" arg-binding-exprs))
      (dolist (arg-binding-expr arg-binding-exprs)
        (unless (and (consp arg-binding-expr)
                     (consp (cdr arg-binding-expr))
                     (member (cddr arg-binding-expr) '(nil (:unused)) :test #'equal))
          (error "Bad function binding ~S" arg-binding-expr))
        (let ((arg-symbol (scan-name world (first arg-binding-expr)))
              (arg-type (scan-type world (second arg-binding-expr)))
              (arg-mode (or (third arg-binding-expr) :const)))
          (setq local-type-env (type-env-add-binding local-type-env arg-symbol arg-type arg-mode))
          (push arg-symbol args)
          (push arg-type arg-types)
          (when (eq arg-mode :unused)
            (push arg-symbol unused-args))))
      (setq args (nreverse args))
      (setq arg-types (nreverse arg-types))
      (setq unused-args (nreverse unused-args))
      (multiple-value-bind (body-codes body-annotated-stmts) (finish-function-code world local-type-env result-type body-statements)
        (when unused-args
          (push (list 'declare (cons 'ignore unused-args)) body-codes))
        (values (cons args body-codes)
                (make-->-type world arg-types result-type)
                body-annotated-stmts)))))


; (lambda ((<var1> <type1> [:unused]) ... (<varn> <typen> [:unused])) <result-type> . <statements>)
(defun scan-lambda (world type-env special-form arg-binding-exprs result-type-expr &rest body-statements)
  (multiple-value-bind (args-and-body-codes type body-annotated-stmts)
                       (scan-function-or-lambda world type-env arg-binding-exprs result-type-expr body-statements)
    (values
     (list 'function (cons 'lambda args-and-body-codes))
     type
     (list* 'expr-annotation:special-form special-form arg-binding-exprs result-type-expr body-annotated-stmts))))


; (if <condition-expr> <true-expr> <false-expr>)
(defun scan-if-expr (world type-env special-form condition-expr true-expr false-expr)
  (multiple-value-bind (condition-code condition-annotated-expr)
                       (scan-typed-value world type-env condition-expr (world-boolean-type world))
    (multiple-value-bind (true-code true-type true-annotated-expr) (scan-value world type-env true-expr)
      (multiple-value-bind (false-code false-type false-annotated-expr) (scan-value world type-env false-expr)
        (handler-bind (((or error warning)
                        #'(lambda (condition)
                            (declare (ignore condition))
                            (format *error-output* "~&~@<~2IWhile processing if with alternatives~_ ~S: ~A and~_ ~S: ~A:~:>~%"
                                    true-expr (print-type-to-string true-type)
                                    false-expr (print-type-to-string false-type)))))
          (let ((type (type-union world true-type false-type)))
            (values
             (list 'if condition-code true-code false-code)
             type
             (list 'expr-annotation:special-form special-form condition-annotated-expr true-annotated-expr false-annotated-expr))))))))


;;; Vectors

(defmacro non-empty-vector (v operation-name)
  `(or ,v (error ,(concatenate 'string operation-name " called on empty vector"))))

(defun make-vector-expr (world special-form element-type element-codes element-annotated-exprs)
  (values
   (if element-codes
     (let ((elements-code (cons 'list element-codes)))
       (if (eq element-type (world-character-type world))
         (if (cdr element-codes)
           (list 'coerce elements-code ''string)
           (list 'string (car element-codes)))
         elements-code))
     (if (eq element-type (world-character-type world))
       ""
       nil))
   (make-vector-type world element-type)
   (list* 'expr-annotation:special-form special-form element-annotated-exprs)))

; (vector <element-expr> ... <element-expr>)
; Makes a vector of one or more elements.
(defun scan-vector-expr (world type-env special-form element-expr &rest element-exprs)
  (multiple-value-bind (element-code element-type element-annotated-expr) (scan-value world type-env element-expr)
    (multiple-value-map-bind (rest-codes rest-annotated-exprs)
                             #'(lambda (element-expr)
                                 (scan-typed-value world type-env element-expr element-type))
                             (element-exprs)
      (make-vector-expr world special-form element-type (cons element-code rest-codes) (cons element-annotated-expr rest-annotated-exprs)))))


; (vector-of <element-type> <element-expr> ... <element-expr>)
; Makes a vector of zero or more elements of the given type.
(defun scan-vector-of (world type-env special-form element-type-expr &rest element-exprs)
  (let ((element-type (scan-type world element-type-expr)))
    (multiple-value-map-bind (element-codes element-annotated-exprs)
                             #'(lambda (element-expr)
                                 (scan-typed-value world type-env element-expr element-type))
                             (element-exprs)
      (make-vector-expr world special-form element-type element-codes element-annotated-exprs))))


; (empty <vector-expr>)
; Returns true if the vector has zero elements.
; This is equivalent to (= (length <vector-expr>) 0) and depicts the same as the latter but
; is implemented more efficiently.
(defun scan-empty (world type-env special-form vector-expr)
  (multiple-value-bind (vector-code vector-type vector-annotated-expr) (scan-vector-value world type-env vector-expr)
    (values
     (if (eq vector-type (world-string-type world))
       `(= (length ,vector-code) 0)
       (list 'endp vector-code))
     (world-boolean-type world)
     (list 'expr-annotation:special-form special-form vector-annotated-expr))))


; (length <vector-expr>)
; Returns the number of elements in the vector.
(defun scan-length (world type-env special-form vector-expr)
  (multiple-value-bind (vector-code vector-type vector-annotated-expr) (scan-vector-value world type-env vector-expr)
    (declare (ignore vector-type))
    (values
     (list 'length vector-code)
     (world-integer-type world)
     (list 'expr-annotation:special-form special-form vector-annotated-expr))))


; (nth <vector-expr> <n-expr>)
; Returns the nth element of the vector.  Throws an error if the vector's length is less than n.
(defun scan-nth (world type-env special-form vector-expr n-expr)
  (multiple-value-bind (vector-code vector-type vector-annotated-expr) (scan-vector-value world type-env vector-expr)
    (multiple-value-bind (n-code n-annotated-expr) (scan-typed-value world type-env n-expr (world-integer-type world))
      (values
       (cond
        ((eq vector-type (world-string-type world))
         `(char ,vector-code ,n-code))
        ((eql n-code 0)
         `(car (non-empty-vector ,vector-code "first")))
        (t (let ((n (gensym "N")))
             `(let ((,n ,n-code))
                (car (non-empty-vector (nthcdr ,n ,vector-code) "nth"))))))
       (vector-element-type vector-type)
       (list 'expr-annotation:special-form special-form vector-annotated-expr n-annotated-expr)))))


; (subseq <vector-expr> <low-expr> [<high-expr>])
; Returns a vector containing elements of the given vector from low-expr to high-expr inclusive.
; high-expr defaults to length-1.
; It is required that 0 <= low-expr <= high-expr+1 <= length.
(defun scan-subseq (world type-env special-form vector-expr low-expr &optional high-expr)
  (let ((integer-type (world-integer-type world)))
    (multiple-value-bind (vector-code vector-type vector-annotated-expr) (scan-vector-value world type-env vector-expr)
      (multiple-value-bind (low-code low-annotated-expr) (scan-typed-value world type-env low-expr integer-type)
        (if high-expr
          (multiple-value-bind (high-code high-annotated-expr) (scan-typed-value world type-env high-expr integer-type)
            (values
             `(subseq ,vector-code ,low-code (1+ ,high-code))
             vector-type
             (list 'expr-annotation:special-form special-form vector-annotated-expr low-annotated-expr high-annotated-expr)))
          (values
           (case low-code
             (0 vector-code)
             (1 (if (eq vector-type (world-string-type world))
                  `(subseq ,vector-code 1)
                  `(cdr (non-empty-vector ,vector-code "rest"))))
             (t `(subseq ,vector-code ,low-code)))
           vector-type
           (list 'expr-annotation:special-form special-form vector-annotated-expr low-annotated-expr nil)))))))


; (append <vector-expr> <vector-expr>)
; Returns a vector contatenating the two given vectors, which must have the same element type.
(defun scan-append (world type-env special-form vector1-expr vector2-expr)
  (multiple-value-bind (vector1-code vector-type vector1-annotated-expr) (scan-vector-value world type-env vector1-expr)
    (multiple-value-bind (vector2-code vector2-annotated-expr) (scan-typed-value world type-env vector2-expr vector-type)
      (values
       (if (eq vector-type (world-string-type world))
         `(concatenate 'string ,vector1-code ,vector2-code)
         (list 'append vector1-code vector2-code))
       vector-type
       (list 'expr-annotation:special-form special-form vector1-annotated-expr vector2-annotated-expr)))))


; (set-nth <vector-expr> <n-expr> <value-expr>)
; Returns a vector containing the same elements of the given vector except that the nth has been replaced
; with value-expr.  n must be between 0 and length-1, inclusive.
(defun scan-set-nth (world type-env special-form vector-expr n-expr value-expr)
  (multiple-value-bind (vector-code vector-type vector-annotated-expr) (scan-vector-value world type-env vector-expr)
    (multiple-value-bind (n-code n-annotated-expr) (scan-typed-value world type-env n-expr (world-integer-type world))
      (multiple-value-bind (value-code value-annotated-expr) (scan-typed-value world type-env value-expr (vector-element-type vector-type))
        (values
         (let ((vector (gensym "V"))
               (n (gensym "N")))
           `(let ((,vector ,vector-code)
                  (,n ,n-code))
              (if (or (< ,n 0) (>= ,n (length ,vector)))
                (error "Range error")
                ,(if (eq vector-type (world-string-type world))
                   `(progn
                      (setq ,vector (copy-seq ,vector))
                      (setf (char ,vector ,n) ,value-code)
                      ,vector)
                   (let ((l (gensym "L")))
                     `(let ((,l (nthcdr ,n ,vector)))
                        (append (ldiff ,vector ,l)
                                (cons ,value-code (cdr ,l)))))))))
         vector-type
         (list 'expr-annotation:special-form special-form vector-annotated-expr n-annotated-expr value-annotated-expr))))))


; (map <vector-expr> <var> <value-expr> [<condition-expr>])
(defun scan-map (world type-env special-form vector-expr var-source value-expr &optional (condition-expr 'true))
  (multiple-value-bind (vector-code vector-type vector-annotated-expr) (scan-vector-value world type-env vector-expr)
    (let* ((var (scan-name world var-source))
           (element-type (vector-element-type vector-type))
           (local-type-env (type-env-add-binding type-env var element-type :const)))
      (multiple-value-bind (value-code value-type value-annotated-expr) (scan-value world local-type-env value-expr)
        (multiple-value-bind (condition-code condition-annotated-expr) (scan-typed-value world local-type-env condition-expr (world-boolean-type world))
          (let* ((result-type (make-vector-type world value-type))
                 (source-is-string (eq element-type (world-character-type world)))
                 (destination-is-string (eq value-type (world-character-type world)))
                 (destination-sequence-type (if destination-is-string 'string 'list))
                 (result-annotated-expr (list 'expr-annotation:special-form special-form vector-annotated-expr var value-annotated-expr condition-annotated-expr)))
            (cond
             ((eq condition-code 't)
              (values
               (if (or source-is-string destination-is-string)
                 `(map ',destination-sequence-type #'(lambda (,var) ,value-code) ,vector-code)
                 `(mapcar #'(lambda (,var) ,value-code) ,vector-code))
               result-type
               (nbutlast result-annotated-expr)))
             ((eq value-expr var-source)
              (assert-true (eq value-type element-type))
              (values
               `(remove-if-not #'(lambda (,var) ,condition-code) ,vector-code)
               result-type
               result-annotated-expr))
             (t 
              (values
               (if (or source-is-string destination-is-string)
                 `(filter-map ',destination-sequence-type #'(lambda (,var) ,condition-code) #'(lambda (,var) ,value-code) ,vector-code)
                 `(filter-map-list #'(lambda (,var) ,condition-code) #'(lambda (,var) ,value-code) ,vector-code))
               result-type
               result-annotated-expr)))))))))


;;; Sets

; Return a function that converts values of the given element-type to integers for storage in a set.
(defun set-in-converter (element-type)
  (ecase (type-kind element-type)
    (:integer #'identity)
    (:character #'char-code)))


; expr is the source code of an expression that generates a value of the given element-type.  Return
; the source code of an expression that generates the corresponding integer for storage in a set of
; the given element-type.
(defun set-in-converter-expr (element-type expr)
  (ecase (type-kind element-type)
    (:integer expr)
    (:character (list 'char-code expr))))


; Return a function that converts integers to values of the given element-type for retrieval from a set.
(defun set-out-converter (element-type)
  (ecase (type-kind element-type)
    (:integer #'identity)
    (:character #'code-char)))


; (set-of <element-type> <element-expr> ... <element-expr>)  ==>
; (set-of-ranges <element-type> <element-expr> nil ... <element-expr> nil)
(defun scan-set-of (world type-env special-form element-type-expr &rest element-exprs)
  (apply #'scan-set-of-ranges
    world type-env special-form element-type-expr
    (mapcan #'(lambda (element-expr)
                (list element-expr nil))
            element-exprs)))


; (set-of-ranges <element-type> <low-expr> <high-expr> ... <low-expr> <high-expr>)
; Makes a set of zero or more elements or element ranges.  Each <high-expr> can be null to indicate a
; one-element range.
(defun scan-set-of-ranges (world type-env special-form element-type-expr &rest element-exprs)
  (let* ((element-type (scan-type world element-type-expr))
         (high t))
    (multiple-value-map-bind (element-codes element-annotated-exprs)
                             #'(lambda (element-expr)
                                 (setq high (not high))
                                 (if (and high (null element-expr))
                                   (values nil nil)
                                   (multiple-value-bind (element-code element-annotated-expr)
                                                        (scan-typed-value world type-env element-expr element-type)
                                     (values (set-in-converter-expr element-type element-code)
                                             element-annotated-expr))))
                             (element-exprs)
      (unless high
        (error "Odd number of set-of-ranges elements: ~S" element-exprs))
      (values
       (cons 'intset-from-ranges element-codes)
       (make-set-type world element-type)
       (list* 'expr-annotation:special-form special-form element-type-expr element-annotated-exprs)))))


;;; Tags

(defparameter *tag-counter* 0)

; (tag <tag> <field-expr1> ... <field-exprn>)
(defun scan-tag-expr (world type-env special-form tag-name &rest value-exprs)
  (let* ((tag (scan-tag world tag-name))
         (type (make-tag-type world tag))
         (fields (tag-fields tag)))
    (unless (= (length value-exprs) (length fields))
      (error "Wrong number of ~A fields given in constructor: ~S" tag-name value-exprs))
    (multiple-value-map-bind (value-codes value-annotated-exprs)
                             #'(lambda (field value-expr)
                                 (scan-typed-value world type-env value-expr (field-type field)))
                             (fields value-exprs)
      (values
       (or (tag-keyword tag)
           (let ((name (tag-name tag)))
             (if (tag-mutable tag)
               (list* 'list (list 'quote name) '(incf *tag-counter*) value-codes)
               (list* 'list (list 'quote name) value-codes))))
       type
       (list* 'expr-annotation:special-form special-form tag value-annotated-exprs)))))


; (& <label> <record-expr>)
; Return the tag field's value.
(defun scan-& (world type-env special-form label record-expr)
  (multiple-value-bind (record-code record-type record-annotated-expr) (scan-tag-value world type-env record-expr)
    (let ((tag (type-tag record-type)))
      (multiple-value-bind (position field-type mutable) (scan-label tag label)
        (declare (ignore mutable))
        (values
         (gen-nth-code position record-code)
         field-type
         (list 'expr-annotation:special-form special-form tag label record-annotated-expr))))))


;;; Unions

; (in <type> <expr>)
(defun scan-in (world type-env special-form type-expr value-expr)
  (let ((type (scan-type world type-expr)))
    (multiple-value-bind (value-code value-type value-annotated-expr) (scan-value world type-env value-expr)
      (type-difference world value-type type)
      (values
       (if (symbolp value-code)
         (type-member-test-code world type value-type value-code)
         (let ((var (gensym "IN")))
           `(let ((,var ,value-code))
              ,(type-member-test-code world type value-type var))))
       (world-boolean-type world)
       (list 'expr-annotation:special-form special-form type type-expr value-annotated-expr)))))

; (not-in <type> <expr>)
(defun scan-not-in (world type-env special-form type-expr value-expr)
  (multiple-value-bind (code type annotated-expr) (scan-in world type-env special-form type-expr value-expr)
    (values
     (list 'not code)
     type
     annotated-expr)))


;;; ------------------------------------------------------------------------------------------------------
;;; STATEMENT EXPRESSIONS


; If source is a list that starts with a statement keyword, return that interned keyword;
; otherwise return nil.
(defun statement? (world source)
  (and (consp source)
       (let ((id (first source)))
         (and (identifier? id)
              (let ((symbol (world-find-symbol world id)))
                (when (get symbol :statement)
                  symbol))))))


; Generate a list of lisp expressions that will execute the given statements.
; type-env is the type environment.
; last is true if these statements' lisp return value becomes the return value of the function if the function falls through.
; live is true if these statements are reachable.
;
; Return three values:
;   A list of codes (a list of lisp expressions)
;   Non-nil if the statement can fall through
;   A list of annotated statements
(defun scan-statements (world type-env statements last live)
  (if statements
    (if live
      (let* ((statement (first statements))
             (rest-statements (rest statements))
             (symbol (statement? world statement)))
        (if symbol
          (apply (get symbol :statement) world type-env rest-statements last symbol (rest statement))
          (multiple-value-bind (statement-code live statement-annotated-expr)
                               (scan-void-value world type-env statement)
            (multiple-value-bind (rest-codes rest-live rest-annotated-stmts) (scan-statements world type-env rest-statements last live)
              (values (cons statement-code rest-codes)
                      rest-live
                      (cons (list (world-intern world 'exec) statement-annotated-expr) rest-annotated-stmts))))))
      (error "Unreachable statements: ~S" statements))
    (values nil live nil)))


; Compute the initial type-env to use for the given general-production's action code.
; The first cell of the type-env gives the production's lhs nonterminal's symbol;
; the remaining cells give the action arguments in order.
(defun general-production-action-env (grammar general-production)
  (let* ((current-indices nil)
         (lhs-general-nonterminal (general-production-lhs general-production))
         (bound-arguments-alist (nonterminal-sample-bound-argument-alist grammar lhs-general-nonterminal)))
    (set-type-env-flag 
     (mapcan
      #'(lambda (general-grammar-symbol)
          (let* ((symbol (general-grammar-symbol-symbol general-grammar-symbol))
                 (index (incf (getf current-indices symbol 0)))
                 (grammar-symbol (instantiate-general-grammar-symbol bound-arguments-alist general-grammar-symbol)))
            (mapcar
             #'(lambda (declaration)
                 (let* ((action-symbol (car declaration))
                        (action-type (cdr declaration))
                        (local-symbol (gensym (symbol-name action-symbol))))
                   (make-type-env-action
                    (list* action-symbol symbol index)
                    local-symbol
                    action-type
                    general-grammar-symbol)))
             (grammar-symbol-signature grammar grammar-symbol))))
      (general-production-rhs general-production))
     :lhs-symbol (general-grammar-symbol-symbol lhs-general-nonterminal))))


; Return the number of arguments that a function returned by compute-action-code
; would expect.
(defun n-action-args (grammar production)
  (let ((n-args 0))
    (dolist (grammar-symbol (production-rhs production))
      (incf n-args (length (grammar-symbol-signature grammar grammar-symbol))))
    n-args))


; Compute the code for evaluating body-expr to obtain the value of one of the
; production's actions.  Verify that the result has the given type and that the
; type is the same as type-expr.
; The code is a lambda-expression that takes as arguments the results of all
; defined actions on the production's rhs.  The arguments are listed in the
; same order as the grammar symbols in the rhs.  If a grammar symbol in the rhs
; has more than one associated action, arguments are used corresponding to all
; of the actions in the same order as they were declared.  If a grammar symbol
; in the rhs has no associated actions, no argument is used for it.
(defun compute-action-code (world grammar production action-symbol type-expr body-expr type)
  (handler-bind (((or error warning)
                  #'(lambda (condition)
                      (declare (ignore condition))
                      (format *error-output* "~&~@<~2IWhile processing action ~A on ~S: ~_~:W~:>~%"
                              action-symbol (production-name production) body-expr))))
    (let ((type2 (scan-type world type-expr)))
      (unless (type= type type2)
        (error "Action declared using type ~A but defined using ~A"
               (print-type-to-string type) (print-type-to-string type2))))
    (let* ((initial-env (general-production-action-env grammar production))
           (args (mapcar #'cadr (cdr initial-env)))
           (body-code (scan-typed-value-or-begin world initial-env body-expr type))
           (named-body-code (name-lambda body-code
                                         (concatenate 'string (symbol-name (production-name production))
                                                      "~" (symbol-name action-symbol))
                                         (world-package world))))
      (gen-lambda args named-body-code))))


; Return a list of all grammar symbols's symbols that are present in at least one expr-annotation:action
; in the annotated expression.  The symbols are returned in no particular order.
(defun annotated-expr-grammar-symbols (annotated-expr)
  (let ((symbols nil))
    (labels
      ((scan (annotated-expr)
         (when (consp annotated-expr)
           (if (eq (first annotated-expr) 'expr-annotation:action)
             (pushnew (general-grammar-symbol-symbol (third annotated-expr)) symbols :test *grammar-symbol-=*)
             (mapc #'scan annotated-expr)))))
      (scan annotated-expr)
      symbols)))


;;; ------------------------------------------------------------------------------------------------------
;;; STATEMENTS


; (exec <expr>)
(defun scan-exec (world type-env rest-statements last special-form expr)
  (multiple-value-bind (statement-code statement-type statement-annotated-expr)
                       (scan-value world type-env expr)
    (multiple-value-bind (rest-codes rest-live rest-annotated-stmts)
                         (scan-statements world type-env rest-statements last (not (eq (type-kind statement-type) :bottom)))
      (values (cons statement-code rest-codes)
              rest-live
              (cons (list special-form statement-annotated-expr) rest-annotated-stmts)))))


; (const <name> <type> <value>)
; (var <name> <type> <value>)
(defun scan-var (world type-env rest-statements last special-form name type-expr value-expr)
  (let* ((symbol (scan-name world name))
         (type (scan-type world type-expr))
         (placeholder-type-env (type-env-add-binding type-env symbol type :unused)))
    (multiple-value-bind (value-code value-annotated-expr) (scan-typed-value world placeholder-type-env value-expr type)
      (let ((local-type-env (type-env-add-binding type-env symbol type (find-keyword special-form))))
        (multiple-value-bind (rest-codes rest-live rest-annotated-stmts)
                             (scan-statements world local-type-env rest-statements last t)
          (values
           (list `(let ((,symbol ,value-code))
                    ,@rest-codes))
           rest-live
           (cons (list special-form name type-expr value-annotated-expr) rest-annotated-stmts)))))))


; (function (<name> (<var1> <type1> [:unused]) ... (<varn> <typen> [:unused])) <result-type> . <statements>)
(defun scan-function (world type-env rest-statements last special-form name-and-arg-binding-exprs result-type-expr &rest body-statements)
  (unless (consp name-and-arg-binding-exprs)
    (error "Bad function name and bindings: ~S" name-and-arg-binding-exprs))
  (let* ((symbol (scan-name world (first name-and-arg-binding-exprs)))
         (placeholder-type-env (type-env-add-binding type-env symbol (world-void-type world) :unused)))
    (multiple-value-bind (args-and-body-codes type body-annotated-stmts)
                         (scan-function-or-lambda world placeholder-type-env (rest name-and-arg-binding-exprs) result-type-expr body-statements)
      (let ((local-type-env (type-env-add-binding type-env symbol type :function)))
        (multiple-value-bind (rest-codes rest-live rest-annotated-stmts)
                             (scan-statements world local-type-env rest-statements last t)
          (values
           (list `(flet ((,symbol ,@args-and-body-codes))
                    ,@rest-codes))
           rest-live
           (cons (list* special-form name-and-arg-binding-exprs result-type-expr body-annotated-stmts) rest-annotated-stmts)))))))


; (<- <name> <value>)
; Mutate the local variable.
(defun scan-<- (world type-env rest-statements last special-form name value-expr)
  (let* ((symbol (scan-name world name))
         (symbol-binding (type-env-get-local type-env symbol)))
    (unless symbol-binding
      (error "Unknown local variable ~A" name))
    (unless (eq (type-env-local-mode symbol-binding) :var)
      (error "Local variable ~A not writable" name))
    (multiple-value-bind (value-code value-annotated-expr) (scan-typed-value world type-env value-expr (type-env-local-type symbol-binding))
      (multiple-value-bind (rest-codes rest-live rest-annotated-stmts)
                           (scan-statements world type-env rest-statements last t)
        (values
         (cons (list 'setq (type-env-local-name symbol-binding) value-code) rest-codes)
         rest-live
         (cons (list special-form name value-annotated-expr) rest-annotated-stmts))))))


; (&= <record-expr> <value-expr>)
; Writes the value of the field.
(defun scan-&= (world type-env rest-statements last special-form label record-expr value-expr)
  (multiple-value-bind (record-code record-type record-annotated-expr) (scan-tag-value world type-env record-expr)
    (let ((tag (type-tag record-type)))
      (multiple-value-bind (position field-type mutable) (scan-label tag label)
        (unless mutable
          (error "Attempt to write to immutable field ~S of ~S" label (tag-name tag)))
        (multiple-value-bind (value-code value-annotated-expr) (scan-typed-value world type-env value-expr field-type)
          (multiple-value-bind (rest-codes rest-live rest-annotated-stmts)
                               (scan-statements world type-env rest-statements last t)
            (values
             (cons (list 'setf (gen-nth-code position record-code) value-code) rest-codes)
             rest-live
             (cons (list special-form tag label record-annotated-expr value-annotated-expr) rest-annotated-stmts))))))))


; (return [<value-expr>])
(defun scan-return (world type-env rest-statements last special-form &optional value-expr)
  (let ((value-code nil)
        (value-annotated-expr nil)
        (type (get-type-env-flag type-env :return)))
    (cond
     (value-expr
      (multiple-value-setq (value-code value-annotated-expr)
        (scan-typed-value world type-env value-expr type)))
     ((not (type= type (world-bottom-type world)))
      (error "Return statement needs a value")))
    (scan-statements world type-env rest-statements last nil)
    (values
     (list (if last
             value-code
             (list* 'return-from
                    (gen-type-env-return-block-name type-env)
                    (and value-code (list value-code)))))
     nil
     (list (list special-form value-annotated-expr)))))


; condition-expr is either (in <type> <var>) or (not-in <type> <var>), a condition expression that constrains
; the type of var in the true branch if criteria is :narrow-true or :narrow-both and in the false branch
; if criteria is :narrow-false or :narrow-both.
; Return two values:
;   A type-env to use if the condition is true;
;   A type-env to use if the condition is false.
(defun scan-narrowing-in-or-not-in (world type-env criteria condition-expr)
  (unless (structured-type? condition-expr '(tuple (member in not-in) t identifier))
    (error "Bad narrowing condition ~S" condition-expr))
  (let ((test (first condition-expr))
        (type-expr (second condition-expr))
        (var-expr (third condition-expr)))
    (multiple-value-bind (var var-type var-annotated-expr) (scan-value world type-env var-expr)
      (declare (ignore var-annotated-expr))
      (assert-true (symbolp var))
      (multiple-value-bind (true-type false-type) (type-difference world var-type (scan-type world type-expr))
        (ecase test
          (in)
          (not-in (rotatef true-type false-type)))
        (values (if (member criteria '(:narrow-true :narrow-both))
                  (type-env-narrow-binding type-env var true-type)
                  type-env)
                (if (member criteria '(:narrow-false :narrow-both))
                  (type-env-narrow-binding type-env var false-type)
                  type-env))))))


; Scan a boolean expression <condition-expr>, which can be one of the following:
;  <expr>         Condition expression <expr>
;  (<key> (in <type> <var>))
;  (<key> (not-in <type> <var>))
;  (:narrow-true (and ([not-]in <type> <var>) ... ([not-]in <type> <var>)))
;  (:narrow-false (or ([not-]in <type> <var>) ... ([not-]in <type> <var>)))
;        where key is :narrow-true, :narrow-false, or :narrow-both
;     Condition expression that constrains the type of var in the true branch if key is :narrow-true or :narrow-both
;     and in the false branch if key is :narrow-false or :narrow-both.
;
; Return four values:
;   The code for the condition;
;   The annotated code for the condition;
;   A type-env to use if the condition is true;
;   A type-env to use if the condition is false.
(defun scan-narrowing-condition (world type-env condition-expr)
  (if (and (consp condition-expr)
           (member (first condition-expr) '(:narrow-true :narrow-false :narrow-both)))
    (if (structured-type? condition-expr '(tuple t (cons (member in not-in and or) t)))
      (let ((criteria (first condition-expr))
            (condition-expr (second condition-expr)))
        (multiple-value-bind (condition-code condition-annotated-expr)
                             (scan-typed-value world type-env condition-expr (world-boolean-type world))
          (multiple-value-bind
            (true-type-env false-type-env)
            (ecase (first condition-expr)
              ((in not-in) (scan-narrowing-in-or-not-in world type-env criteria condition-expr))
              (and (unless (eq criteria :narrow-true)
                     (error "Only :narrow-true may be used with a conjunction"))
                   (let ((true-type-env type-env))
                     (dolist (subcondition-expr (cdr condition-expr))
                       (setq true-type-env (scan-narrowing-in-or-not-in world true-type-env criteria subcondition-expr)))
                     (values true-type-env type-env)))
              (or (unless (eq criteria :narrow-false)
                    (error "Only :narrow-false may be used with a disjunction"))
                  (let ((false-type-env type-env))
                    (dolist (subcondition-expr (cdr condition-expr))
                      (setq false-type-env (nth-value 1 (scan-narrowing-in-or-not-in world false-type-env criteria subcondition-expr))))
                    (values type-env false-type-env))))
            (values condition-code condition-annotated-expr true-type-env false-type-env))))
      (error "Bad narrowing condition ~S" condition-expr))
    (multiple-value-bind (condition-code condition-annotated-expr)
                         (scan-typed-value world type-env condition-expr (world-boolean-type world))
      (values condition-code condition-annotated-expr type-env type-env))))


; (rwhen <condition-expr> . <true-statements>)
; Same as when except that checks that true-statements cannot fall through and generates more efficient code.
(defun scan-rwhen (world type-env rest-statements last special-form condition-expr &rest true-statements)
  (multiple-value-bind (condition-code condition-annotated-expr true-type-env false-type-env)
                       (scan-narrowing-condition world type-env condition-expr)
    (multiple-value-bind (true-codes true-live true-annotated-stmts) (scan-statements world true-type-env true-statements last t)
      (when true-live
        (error "rwhen statements ~S must not fall through" true-statements))
      (multiple-value-bind (rest-codes rest-live rest-annotated-stmts)
                           (scan-statements world false-type-env rest-statements last t)
        (values (list (list 'if condition-code (gen-progn true-codes) (gen-progn rest-codes)))
                rest-live
                (cons (list special-form (cons condition-annotated-expr true-annotated-stmts)) rest-annotated-stmts))))))


; (when <condition-expr> . <true-statements>)
(defun scan-when (world type-env rest-statements last special-form condition-expr &rest true-statements)
  (scan-cond world type-env rest-statements last special-form (cons condition-expr true-statements)))


; (if <condition-expr> <true-statement> <false-statement>)
(defun scan-if-stmt (world type-env rest-statements last special-form condition-expr true-statement false-statement)
  (scan-cond world type-env rest-statements last special-form (list condition-expr true-statement) (list nil false-statement)))


; Generate and optimize a cond statement with the given cases.
(defun gen-cond-code (cases)
  (cond
   ((endp cases) nil)
   ((endp (cdr cases))
    (cons 'when (car cases)))
   ((and (endp (cddr cases)) (eq (car (second cases)) t) (endp (cddr (first cases))) (endp (cddr (second cases))))
    (list 'if (first (first cases)) (second (first cases)) (second (second cases))))
   (t (cons 'cond cases))))


; (cond (<condition-expr> . <statements>) ... (<condition-expr> . <statements>) [(nil . <statements>)])
; <condition-expr> can be one of the following:
;  nil            Always true; used for an "else" clause
;  true           Same as nil
;  <expr>         Condition expression <expr>
;  (<key> (in <type> <var>))
;  (<key> (not-in <type> <var>))
;  (:narrow-true (and ([not-]in <type> <var>) ... ([not-]in <type> <var>)))
;  (:narrow-false (or ([not-]in <type> <var>) ... ([not-]in <type> <var>)))
;        where key is :narrow-true, :narrow-false, or :narrow-both
;     Condition expression that constrains the type of var in the true branch if key is :narrow-true or :narrow-both
;     and in the false branch if key is :narrow-false or :narrow-both.
(defun scan-cond (world type-env rest-statements last special-form &rest cases)
  (unless cases
    (error "Empty cond statement"))
  (let ((local-type-env type-env)
        (nested-last (and last (null rest-statements)))
        (case-codes nil)
        (annotated-cases nil)
        (any-live nil)
        (found-default-case nil))
    (dolist (case cases)
      (unless (consp case)
        (error "Bad cond case: ~S" case))
      (when found-default-case
        (error "Cond case follows default case: ~S" cases))
      (let ((condition-expr (first case)))
        (multiple-value-bind (condition-code condition-annotated-expr true-type-env false-type-env)
                             (if (member condition-expr '(nil true))
                               (values t nil local-type-env local-type-env)
                               (scan-narrowing-condition world local-type-env condition-expr))
          (when (eq condition-code t)
            (if (cdr cases)
              (setq found-default-case t)
              (error "Cond statement consisting only of an else case: ~S" cases)))
          (multiple-value-bind (codes live annotated-stmts) (scan-statements world true-type-env (rest case) nested-last t)
            (push (cons condition-code codes) case-codes)
            (push (cons condition-annotated-expr annotated-stmts) annotated-cases)
            (when live
              (setq any-live t)))
          (setq local-type-env false-type-env))))
    (unless found-default-case
      (setq any-live t))
    (multiple-value-bind (rest-codes rest-live rest-annotated-stmts)
                         (scan-statements world type-env rest-statements last any-live)
      (values (cons (gen-cond-code (nreverse case-codes)) rest-codes)
              rest-live
              (cons (cons special-form (nreverse annotated-cases)) rest-annotated-stmts)))))


; (while <condition-expr> . <statements>)
(defun scan-while (world type-env rest-statements last special-form condition-expr &rest loop-statements)
  (multiple-value-bind (condition-code condition-annotated-expr)
                       (scan-typed-value world type-env condition-expr (world-boolean-type world))
    (multiple-value-bind (loop-codes loop-live loop-annotated-stmts) (scan-statements world type-env loop-statements nil t)
      (unless loop-live
        (warn "While loop can execute at most once: ~S ~S" condition-expr loop-statements))
      (let ((infinite (and (constantp condition-code) (symbolp condition-code) condition-code)))
        (multiple-value-bind (rest-codes rest-live rest-annotated-stmts)
                             (scan-statements world type-env rest-statements last (not infinite))
          (values
           (cons (if infinite
                   (cons 'loop loop-codes)
                   `(do ()
                        ((not ,condition-code))
                      ,@loop-codes))
                 rest-codes)
           rest-live
           (cons (list* special-form condition-annotated-expr loop-annotated-stmts) rest-annotated-stmts)))))))


; (assert <condition-expr>)
; Used to declare conditions that are known to be true if the semantics function correctly.  Don't use this to
; verify user input.
(defun scan-assert (world type-env rest-statements last special-form condition-expr)
  (multiple-value-bind (condition-code condition-annotated-expr)
                       (scan-typed-value world type-env condition-expr (world-boolean-type world))
    (multiple-value-bind (rest-codes rest-live rest-annotated-stmts) (scan-statements world type-env rest-statements last t)
      (values (cons (list 'assert condition-code) rest-codes)
              rest-live
              (cons (list special-form condition-annotated-expr) rest-annotated-stmts)))))


(defconstant *semantic-exception-type-name* 'semantic-exception)

; (throw <value-expr>)
; <value-expr> must have type *semantic-exception-type-name*, which must be the name of some user-defined type in the environment.
(defun scan-throw (world type-env rest-statements last special-form value-expr)
  (multiple-value-bind (value-code value-annotated-expr)
                       (scan-typed-value world type-env value-expr (scan-type world *semantic-exception-type-name*))
    (scan-statements world type-env rest-statements last nil)
    (values
     (list (list 'throw :semantic-exception value-code))
     nil
     (list (list special-form value-annotated-expr)))))


; (catch <body-statements> (<var> [:unused]) . <handler-statements>)
(defun scan-catch (world type-env rest-statements last special-form body-statements arg-binding-expr &rest handler-statements)
  (multiple-value-bind (body-codes body-live body-annotated-stmts) (scan-statements world type-env body-statements nil t)
    (unless (and (consp arg-binding-expr)
                 (member (cdr arg-binding-expr) '(nil (:unused)) :test #'equal))
      (error "Bad catch binding ~S" arg-binding-expr))
    (let* ((nested-last (and last (null rest-statements)))
           (arg-symbol (scan-name world (first arg-binding-expr)))
           (arg-type (scan-type world *semantic-exception-type-name*))
           (type-env (type-env-add-binding type-env arg-symbol arg-type :const)))
      (multiple-value-bind (handler-codes handler-live handler-annotated-stmts) (scan-statements world type-env handler-statements nested-last t)
        (multiple-value-bind (rest-codes rest-live rest-annotated-stmts)
                             (scan-statements world type-env rest-statements last (or body-live handler-live))
          (let ((code
                 `(block nil
                    (let ((,arg-symbol (catch :semantic-exception ,@body-codes ,@(when body-live '((return))))))
                      ,@(and (eq (second arg-binding-expr) :unused) `((declare (ignore ,arg-symbol))))
                      ,@handler-codes))))
            (values (cons code rest-codes)
                    rest-live
                    (cons (list* special-form body-annotated-stmts arg-binding-expr handler-annotated-stmts) rest-annotated-stmts))))))))


(defun case-error ()
  (error "No case chosen"))

; (case <value-expr> (key <type> . <statements>) ... (keyword <type> . <statements>))
; where each key is one of:
;    :select    No special action
;    :narrow    Narrow the type of <value-expr>, which must be a variable, to this case's <type>
;    :otherwise Catch-all else case; <type> should be either nil or the remaining catch-all type
(defun scan-case (world type-env rest-statements last special-form value-expr &rest cases)
  (multiple-value-bind (value-code value-type value-annotated-expr) (scan-value world type-env value-expr)
    (handler-bind (((or error warning)
                    #'(lambda (condition)
                        (declare (ignore condition))
                        (format *error-output* "~@<In case ~S: ~A ~_~S~:>" value-expr (print-type-to-string value-type) cases))))
      (let ((var (if (symbolp value-code) value-code (gensym "CASE")))
            (nested-last (and last (null rest-statements))))
        (labels
          ((process-remaining-cases (cases remaining-type)
             (if cases
               (let ((case (car cases))
                     (cases (cdr cases)))
                 (unless (and (consp case) (consp (cdr case)) (member (car case) '(:select :narrow :otherwise)))
                   (error "Bad case ~S" case))
                 (let ((key (first case))
                       (type-expr (second case))
                       (statements (cddr case)))
                   (if (eq key :otherwise)
                     (progn
                       (when cases
                         (error "Otherwise case must be the last one"))
                       (when type-expr
                         (let ((type (scan-type world type-expr)))
                           (unless (type= type remaining-type)
                             (error "Otherwise case type ~A given but ~A expected"
                                    (print-type-to-string type) (print-type-to-string remaining-type)))))
                       (when (type= remaining-type (world-bottom-type world))
                         (error "Otherwise case not reached"))
                       (multiple-value-bind (statements-codes statements-live statements-annotated-stmts)
                                            (scan-statements world type-env statements nested-last t)
                         (values (list (cons t statements-codes))
                                 statements-live
                                 (list (list* key type-expr statements-annotated-stmts)))))
                     (multiple-value-bind (type remaining-type) (type-difference world remaining-type (scan-type world type-expr))
                       (let ((condition-code (type-member-test-code world type value-type var)))
                         (multiple-value-bind (remaining-code remaining-live remaining-annotated-stmts)
                                              (process-remaining-cases cases remaining-type)
                           (ecase key
                             (:select
                              (multiple-value-bind (statements-codes statements-live statements-annotated-stmts)
                                                   (scan-statements world type-env statements nested-last t)
                                (values (cons (cons condition-code statements-codes) remaining-code)
                                        (or statements-live remaining-live)
                                        (cons (list* key type-expr statements-annotated-stmts) remaining-annotated-stmts))))
                             (:narrow
                               (unless (equal var value-code)
                                 (error "const and var cases can only be used when dispatching on a variable"))
                               (multiple-value-bind (statements-codes statements-live statements-annotated-stmts)
                                                    (scan-statements world (type-env-narrow-binding type-env var type) statements nested-last t)
                                 (values (cons (cons condition-code statements-codes) remaining-code)
                                         (or statements-live remaining-live)
                                         (cons (list* key type-expr statements-annotated-stmts) remaining-annotated-stmts)))))))))))
               (if (type= remaining-type (world-bottom-type world))
                 (values '((t (case-error))) nil nil)
                 (error "Type ~A not considered in case" remaining-type)))))
          
          (multiple-value-bind (cases-code cases-live cases-annotated-stmts) (process-remaining-cases cases value-type)
            (multiple-value-bind (rest-codes rest-live rest-annotated-stmts)
                                 (scan-statements world type-env rest-statements last cases-live)
              (values
               (cons (if (equal var value-code)
                       (cons 'cond cases-code)
                       `(let ((,var ,value-code))
                          (cond ,@cases-code)))
                     rest-codes)
               rest-live
               (cons (list* special-form value-annotated-expr cases-annotated-stmts) rest-annotated-stmts)))))))))


;;; ------------------------------------------------------------------------------------------------------
;;; COMMANDS

; (%highlight <highlight> <command> ... <command>)
; Evaluate the given commands.  <highlight> is a hint for printing.
(defun scan-%highlight (world grammar-info-var highlight &rest commands)
  (declare (ignore highlight))
  (scan-commands world grammar-info-var commands))


; (%... ...)
; Ignore any command that starts with a %.  These commands are hints for printing.
(defun scan-% (world grammar-info-var &rest rest)
  (declare (ignore world grammar-info-var rest)))


; (deftag <name> (<name1> <type1>) ... (<namen> <typen>))
; Create the immutable tag in the world and set its contents.
; Do not evaluate the field and type expressions yet; that will be done by eval-tags-types.
(defun scan-deftag (world grammar-info-var name &rest fields)
  (declare (ignore grammar-info-var))
  (add-tag world name nil fields :reference))


; (defrecord <name> (<name1> <type1> [:const | :var]) ... (<namen> <typen> [:const | :var]))
; Create the mutable tag in the world and set its contents.
; Do not evaluate the field and type expressions yet; that will be done by eval-tags-types.
; Fields are immutable unless :var is specified.
(defun scan-defrecord (world grammar-info-var name &rest fields)
  (declare (ignore grammar-info-var))
  (add-tag world name t fields :reference))


; (deftype <name> <type>)
; Create the type in the world and set its contents.
(defun scan-deftype (world grammar-info-var name type-expr)
  (declare (ignore grammar-info-var))
  (let* ((symbol (scan-name world name))
         (type (scan-type world type-expr t)))
    (unless (typep type 'type)
      (error "~:W undefined in type definition of ~A" type-expr symbol))
    (add-type-name world type symbol t)))


; (define <name> <type> <value>)
; (defun <name> (-> (<type1> ... <typen>) <result-type>) (lambda ((<arg1> <type1>) ... (<argn> <typen>)) <result-type> . <statements>))
; Create the variable in the world but do not evaluate its type or value yet.
(defun scan-define (world grammar-info-var name type-expr value-expr)
  (declare (ignore grammar-info-var))
  (let ((symbol (scan-name world name)))
    (unless (eq (get symbol :value-expr *get2-nonce*) *get2-nonce*)
      (error "Attempt to redefine variable ~A" symbol))
    (setf (get symbol :value-expr) value-expr)
    (setf (get symbol :type-expr) type-expr)
    (export-symbol symbol)))


; (set-grammar <name>)
; Set the current grammar to the grammar or lexer with the given name.
(defun scan-set-grammar (world grammar-info-var name)
  (let ((grammar-info (world-grammar-info world name)))
    (unless grammar-info
      (error "Unknown grammar ~A" name))
    (setf (car grammar-info-var) grammar-info)))


; (clear-grammar)
; Clear the current grammar.
(defun scan-clear-grammar (world grammar-info-var)
  (declare (ignore world))
  (setf (car grammar-info-var) nil))


; Get the grammar-info-var's grammar.  Signal an error if there isn't one.
(defun checked-grammar (grammar-info-var)
  (let ((grammar-info (car grammar-info-var)))
    (if grammar-info
      (grammar-info-grammar grammar-info)
      (error "Grammar needed"))))


; (declare-action <action-name> <general-grammar-symbol> <type> <n-productions>)
(defun scan-declare-action (world grammar-info-var action-name general-grammar-symbol-source type-expr n-productions)
  (declare (ignore n-productions))
  (let* ((grammar (checked-grammar grammar-info-var))
         (action-symbol (scan-name world action-name))
         (general-grammar-symbol (grammar-parametrization-intern grammar general-grammar-symbol-source)))
    (declare-action grammar general-grammar-symbol action-symbol type-expr)
    (dolist (grammar-symbol (general-grammar-symbol-instances grammar general-grammar-symbol))
      (push (cons (car grammar-info-var) grammar-symbol) (symbol-action action-symbol)))
    (export-symbol action-symbol)))


; (action <action-name> <production-name> <type> <n-productions> <value>)
; (actfun <action-name> <production-name> <type> <n-productions> <value>)
(defun scan-action (world grammar-info-var action-name production-name type-expr n-productions value-expr)
  (declare (ignore n-productions))
  (let ((grammar (checked-grammar grammar-info-var))
        (action-symbol (world-intern world action-name)))
    (define-action grammar production-name action-symbol type-expr value-expr)))


; (terminal-action <action-name> <terminal> <lisp-function>)
(defun scan-terminal-action (world grammar-info-var action-name terminal function)
  (let ((grammar (checked-grammar grammar-info-var))
        (action-symbol (world-intern world action-name)))
    (define-terminal-action grammar terminal action-symbol (symbol-function function))))


;;; ------------------------------------------------------------------------------------------------------
;;; INITIALIZATION

(defparameter *default-specials*
  '((:preprocess
     (? preprocess-?)
     (define preprocess-define)
     (action preprocess-action)
     (grammar preprocess-grammar)
     (line-grammar preprocess-line-grammar)
     (lexer preprocess-lexer)
     (grammar-argument preprocess-grammar-argument)
     (production preprocess-production)
     (rule preprocess-rule)
     (exclude preprocess-exclude))
    
    (:command
     (%highlight scan-%highlight depict-%highlight) ;For internal use only; use ? instead.
     (%section scan-% depict-%section)
     (%subsection scan-% depict-%subsection)
     (%text scan-% depict-%text)
     (grammar-argument scan-% depict-grammar-argument)
     (%rule scan-% depict-%rule)
     (%charclass scan-% depict-%charclass)
     (%print-actions scan-% depict-%print-actions)
     (deftag scan-deftag depict-deftag)
     (defrecord scan-defrecord depict-deftag)
     (deftype scan-deftype depict-deftype)
     (define scan-define depict-define)
     (defun scan-define depict-defun)    ;Occurs from desugaring a function define
     (set-grammar scan-set-grammar depict-set-grammar)
     (clear-grammar scan-clear-grammar depict-clear-grammar)
     (declare-action scan-declare-action depict-declare-action)
     (action scan-action depict-action)
     (actfun scan-action depict-actfun)
     (terminal-action scan-terminal-action depict-terminal-action))
    
    (:statement
     (exec scan-exec depict-exec)
     (const scan-var depict-var)
     (var scan-var depict-var)
     (function scan-function depict-function)
     (<- scan-<- depict-<-)
     (&= scan-&= depict-&=)
     (return scan-return depict-return)
     (rwhen scan-rwhen depict-cond)
     (when scan-when depict-cond)
     (if scan-if-stmt depict-cond)
     (cond scan-cond depict-cond)
     (while scan-while depict-while)
     (assert scan-assert depict-assert)
     (throw scan-throw depict-throw)
     (catch scan-catch depict-catch)
     (case scan-case depict-case))
    
    (:special-form
     ;;Constants
     (bottom scan-bottom depict-bottom)
     (todo scan-bottom depict-todo)
     (hex scan-hex depict-hex)
     
     ;;Expressions
     (expt scan-expt depict-expt)
     (= scan-= depict-comparison)
     (/= scan-/= depict-comparison)
     (< scan-< depict-comparison)
     (> scan-> depict-comparison)
     (<= scan-<= depict-comparison)
     (>= scan->= depict-comparison)
     (cascade scan-cascade depict-cascade)
     (and scan-and depict-and-or-xor)
     (or scan-or depict-and-or-xor)
     (xor scan-xor depict-and-or-xor)
     (lambda scan-lambda depict-lambda)
   #|(if scan-if-expr depict-if-expr)|# ;Fully functional but turned off for stylistic reasons
     
     ;;Vectors
     (vector scan-vector-expr depict-vector-expr)
     (vector-of scan-vector-of depict-vector-expr)
     (empty scan-empty depict-empty)
     (length scan-length depict-length)
     (nth scan-nth depict-nth)
     (subseq scan-subseq depict-subseq)
     (append scan-append depict-append)
     (set-nth scan-set-nth depict-set-nth)
     (map scan-map depict-map)
     
     ;;Sets
     (set-of scan-set-of depict-set-of-ranges)
     (set-of-ranges scan-set-of-ranges depict-set-of-ranges)
     
     ;;Tags
     (tag scan-tag-expr depict-tag-expr)
     (& scan-& depict-&)
     
     ;;Unions
     (in scan-in depict-in)
     (not-in scan-not-in depict-not-in))
    
    (:type-constructor
     (-> scan--> depict-->)
     (vector scan-vector depict-vector)
     (set scan-set depict-set)
     (tag scan-tag-type depict-tag-type)
     (union scan-union depict-union))))


(defparameter *default-primitives*
  '((neg (-> (integer) integer) #'- :unary :minus nil %suffix% %suffix%)
    (* (-> (integer integer) integer) #'* :infix :cartesian-product-10 nil %factor% %factor% %factor%)
    (mod (-> (integer integer) integer) #'mod :infix ((:semantic-keyword "mod")) t %factor% %factor% %unary%)
    (+ (-> (integer integer) integer) #'+ :infix "+" t %term% %term% %term%)
    (- (-> (integer integer) integer) #'- :infix :minus t %term% %term% %factor%)
    
    (rat-neg (-> (rational) rational) #'- :unary "-" nil %suffix% %suffix%)
    (rat* (-> (rational rational) rational) #'* :infix :cartesian-product-10 nil %factor% %factor% %factor%)
    (rat/ (-> (rational rational) rational) #'/ :infix "/" nil %factor% %factor% %unary%)
    (rat+ (-> (rational rational) rational) #'+ :infix "+" t %term% %term% %term%)
    (rat- (-> (rational rational) rational) #'- :infix :minus t %term% %term% %factor%)
    (floor (-> (rational) integer) #'floor :unary :left-floor-10 :right-floor-10 %primary% %expr%)
    (ceiling (-> (rational) integer) #'ceiling :unary :left-ceiling-10 :right-ceiling-10 %primary% %expr%)
    
    (not (-> (boolean) boolean) #'not :unary ((:semantic-keyword "not") " ") nil %not% %not%)
    
    (bitwise-and (-> (integer integer) integer) #'logand)
    (bitwise-or (-> (integer integer) integer) #'logior)
    (bitwise-xor (-> (integer integer) integer) #'logxor)
    (bitwise-shift (-> (integer integer) integer) #'ash)
    
    (real-to-float64 (-> (rational) finite-float64) #'rational-to-float64)
    (truncate-finite-float64 (-> (finite-float64) integer) #'truncate-finite-float64)
    
    (float64-compare (-> (float64 float64) order) #'float64-compare)
    (float64-abs (-> (float64 float64) float64) #'float64-abs)
    (float64-negate (-> (float64) float64) #'float64-neg)
    (float64-add (-> (float64 float64) float64) #'float64-add)
    (float64-subtract (-> (float64 float64) float64) #'float64-subtract)
    (float64-multiply (-> (float64 float64) float64) #'float64-multiply)
    (float64-divide (-> (float64 float64) float64) #'float64-divide)
    (float64-remainder (-> (float64 float64) float64) #'float64-remainder)
    
    (code-to-character (-> (integer) character) #'code-char)
    (character-to-code (-> (character) integer) #'char-code)
    
    (integer-set-length (-> (integer-set) integer) #'intset-length :unary "|" "|" %primary% %expr%)
    (integer-set-min (-> (integer-set) integer) #'integer-set-min :unary ((:semantic-keyword "min") " ") nil %min-max% %prefix%)
    (integer-set-max (-> (integer-set) integer) #'integer-set-max :unary ((:semantic-keyword "max") " ") nil %min-max% %prefix%)
    (integer-set-intersection (-> (integer-set integer-set) integer-set) #'intset-intersection :infix :intersection-10 t %factor% %factor% %factor%)
    (integer-set-union (-> (integer-set integer-set) integer-set) #'intset-union :infix :union-10 t %term% %term% %term%)
    (integer-set-difference (-> (integer-set integer-set) integer-set) #'intset-difference :infix :minus t %term% %term% %factor%)
    (integer-set-member (-> (integer integer-set) boolean) #'integer-set-member :infix :member-10 t %relational% %term% %term%)
    
    (character-set-length (-> (character-set) integer) #'intset-length :unary "|" "|" %primary% %expr%)
    (character-set-min (-> (character-set) character) #'character-set-min :unary ((:semantic-keyword "min") " ") nil %min-max% %prefix%)
    (character-set-max (-> (character-set) character) #'character-set-max :unary ((:semantic-keyword "max") " ") nil %min-max% %prefix%)
    (character-set-intersection (-> (character-set character-set) character-set) #'intset-intersection :infix :intersection-10 t %factor% %factor% %factor%)
    (character-set-union (-> (character-set character-set) character-set) #'intset-union :infix :union-10 t %term% %term% %term%)
    (character-set-difference (-> (character-set character-set) character-set) #'intset-difference :infix :minus t %term% %term% %factor%)
    (character-set-member (-> (character character-set) boolean) #'character-set-member :infix :member-10 t %relational% %term% %term%)
    
    (digit-value (-> (character) integer) #'digit-char-36)
    (is-initial-identifier-character (-> (character) boolean) #'initial-identifier-character?)
    (is-continuing-identifier-character (-> (character) boolean) #'continuing-identifier-character?)))


;;; Partial order of primitives for deciding when to depict parentheses.
(defparameter *primitive-level* (make-partial-order))
(def-partial-order-element *primitive-level* %primary%)                                          ;id, constant, (e), tag<...>, |e|, action
(def-partial-order-element *primitive-level* %suffix% %primary%)                                 ;f(...), a[i], a[i...j], a[i<-v], a.l
(def-partial-order-element *primitive-level* %prefix% %primary%)                                 ;-e, new tag<...>, a^b
(def-partial-order-element *primitive-level* %min-max% %prefix%)                                 ;min, max
(def-partial-order-element *primitive-level* %unary% %suffix% %prefix%)                          ;
(def-partial-order-element *primitive-level* %factor% %unary%)                                   ;/, *, intersection
(def-partial-order-element *primitive-level* %term% %factor%)                                    ;+, -, append, union, set difference
(def-partial-order-element *primitive-level* %relational% %term% %min-max%)                      ;<, <=, >, >=, =, /=, is, member
(def-partial-order-element *primitive-level* %not% %relational%)                                 ;not
(def-partial-order-element *primitive-level* %and% %not%)                                        ;and
(def-partial-order-element *primitive-level* %or% %not%)                                         ;or
(def-partial-order-element *primitive-level* %xor% %not%)                                        ;xor
(def-partial-order-element *primitive-level* %logical% %and% %or% %xor%)                         ;
(def-partial-order-element *primitive-level* %expr% %logical%)                                   ;if


; Return the tail end of the lambda list for make-primitive.  The returned list always starts with
; an appearance constant and is followed by additional keywords as appropriate for that appearance.
(defun process-primitive-spec-appearance (name primitive-spec-appearance)
  (if primitive-spec-appearance
    (let ((appearance (first primitive-spec-appearance))
          (args (rest primitive-spec-appearance)))
      (cons
       appearance
       (ecase appearance
         (:global
          (assert-type args (tuple t symbol))
          (list :markup1 (first args) :level (symbol-value (second args))))
         (:infix
          (assert-type args (tuple t bool symbol symbol symbol))
          (list :markup1 (first args) :markup2 (second args) :level (symbol-value (third args))
                :level1 (symbol-value (fourth args)) :level2 (symbol-value (fifth args))))
         (:unary
          (assert-type args (tuple t t symbol symbol))
          (list :markup1 (first args) :markup2 (second args) :level (symbol-value (third args))
                :level1 (symbol-value (fourth args))))
         (:phantom
          (assert-true (null args))
          (list :level %primary%)))))
    (let ((name (symbol-lower-mixed-case-name name)))
      `(:global :markup1 ((:global-variable ,name)) :markup2 ,name :level ,%primary%))))


; Create a world with the given name and set up the built-in properties of its symbols.
; conditionals is an association list of (conditional . highlight), where conditional is a symbol
; and highlight is either:
;   a style keyword:   Use that style to highlight the contents of any (? conditional ...) commands
;   nil:               Include the contents of any (? conditional ...) commands without highlighting them
;   delete:            Don't include the contents of (? conditional ...) commands
(defun init-world (name conditionals)
  (assert-type conditionals (list (cons symbol (or null keyword (eql delete)))))
  (let ((world (make-world name)))
    (setf (world-conditionals world) conditionals)
    (dolist (specials-list *default-specials*)
      (let ((property (car specials-list)))
        (dolist (special-spec (cdr specials-list))
          (apply #'add-special
            property
            (world-intern world (first special-spec))
            (rest special-spec)))))
    (dolist (primitive-spec *default-primitives*)
      (let ((name (world-intern world (first primitive-spec))))
        (apply #'declare-primitive
          name
          (second primitive-spec)
          (third primitive-spec)
          (process-primitive-spec-appearance name (cdddr primitive-spec)))))
    
    ;Define simple types
    (add-type-name world
                   (setf (world-false-type world) (make-tag-type world (setf (world-false-tag world) (add-tag world 'false nil nil nil))))
                   (world-intern world 'false-type)
                   nil)
    (add-type-name world
                   (setf (world-true-type world) (make-tag-type world (setf (world-true-tag world) (add-tag world 'true nil nil nil))))
                   (world-intern world 'true-type)
                   nil)
    (setf (world-denormalized-false-type world) (make-denormalized-tag-type world (world-false-tag world)))
    (setf (world-denormalized-true-type world) (make-denormalized-tag-type world (world-true-tag world)))
    (assert-true (< (type-serial-number (world-false-type world)) (type-serial-number (world-true-type world))))
    (setf (world-boxed-boolean-type world)
          (make-type world :union nil (list (world-false-type world) (world-true-type world)) 'eq nil))
    (flet ((make-simple-type (name kind =-name /=-name)
             (let ((type (make-type world kind nil nil =-name /=-name)))
               (add-type-name world type (world-intern world name) nil)
               type)))
      (setf (world-bottom-type world) (make-simple-type 'bottom-type :bottom nil nil))
      (setf (world-void-type world) (make-simple-type 'void :void nil nil))
      (setf (world-boolean-type world) (make-simple-type 'boolean :boolean 'boolean= nil))
      (setf (world-integer-type world) (make-simple-type 'integer :integer '= '/=))
      (setf (world-rational-type world) (make-simple-type 'rational :rational '= '/=))
      (setf (world-finite64-type world) (make-simple-type 'nonzero-finite-float64 :finite64 '= '/=))
      (setf (world-character-type world) (make-simple-type 'character :character 'char= 'char/=))
      (let ((string-type (make-type world :string nil (list (world-character-type world)) 'string= 'string/=)))
        (add-type-name world string-type (world-intern world 'string) nil)
        (setf (world-string-type world) string-type)))
    (add-type-name world (make-set-type world (world-integer-type world)) (world-intern world 'integer-set) nil)
    (add-type-name world (make-set-type world (world-character-type world)) (world-intern world 'character-set) nil)
    
    ;Define order and floating-point types
    (let ((order-types (mapcar
                        #'(lambda (tag-name)
                            (make-tag-type world (add-tag world tag-name nil nil nil)))
                        '(less equal greater unordered)))
          (float64-tag-types (mapcar
                              #'(lambda (tag-name)
                                  (make-tag-type world (add-tag world tag-name nil nil nil)))
                              '(+zero -zero +infinity -infinity nan))))
      (add-type-name world (apply #'make-union-type world order-types) (world-intern world 'order) nil)
      (add-type-name world (apply #'make-union-type world (world-finite64-type world) float64-tag-types)
                     (world-intern world 'float64) nil)
      (add-type-name world (make-union-type world (world-finite64-type world) (first float64-tag-types) (second float64-tag-types))
                     (world-intern world 'finite-float64) nil))
    world))


(defun print-world (world &optional (stream t) (all t))
  (pprint-logical-block (stream nil)
    (labels
      ((default-print-contents (symbol value stream)
         (declare (ignore symbol))
         (write value :stream stream))
       
       (print-symbols-and-contents (property title separator print-contents)
         (let ((symbols (all-world-external-symbols-with-property world property)))
           (when symbols
             (pprint-logical-block (stream symbols)
               (write-string title stream)
               (pprint-indent :block 2 stream)
               (pprint-newline :mandatory stream)
               (loop
                 (let ((symbol (pprint-pop)))
                   (pprint-logical-block (stream nil)
                     (if separator
                       (format stream "~A ~@_~:I~A " symbol separator)
                       (format stream "~A " symbol))
                     (funcall print-contents symbol (get symbol property) stream)))
                 (pprint-exit-if-list-exhausted)
                 (pprint-newline :mandatory stream)))
             (pprint-newline :mandatory stream)
             (pprint-newline :mandatory stream)))))
      
      (when all
        (print-symbols-and-contents
         :preprocess "Preprocessor actions:" "::" #'default-print-contents)
        (print-symbols-and-contents
         :command "Commands:" "::" #'default-print-contents)
        (print-symbols-and-contents
         :statement "Special Forms:" "::" #'default-print-contents)
        (print-symbols-and-contents
         :special-form "Special Forms:" "::" #'default-print-contents)
        (print-symbols-and-contents
         :primitive "Primitives:" ":"
         #'(lambda (symbol primitive stream)
             (declare (ignore symbol))
             (let ((type (primitive-type primitive)))
               (if type
                 (print-type type stream)
                 (format stream "~@<<<~;~W~;>>~:>" (primitive-type-expr primitive))))
             (format stream " ~_= ~@<<~;~W~;>~:>" (primitive-value-code primitive))))
        (print-symbols-and-contents
         :type-constructor "Type Constructors:" "::" #'default-print-contents))
      
      (print-symbols-and-contents
       :tag "Tags:" "=="
       #'(lambda (symbol tag stream)
           (declare (ignore symbol))
           (print-tag tag stream)))
      (print-symbols-and-contents
       :deftype "Types:" "=="
       #'(lambda (symbol type stream)
           (if type
             (print-type type stream (eq symbol (type-name type)))
             (format stream "<forward-referenced>"))))
      (print-symbols-and-contents
       :value-expr "Values:" ":"
       #'(lambda (symbol value-expr stream)
           (let ((type (symbol-type symbol)))
             (if type
               (print-type type stream)
               (format stream "~@<<<~;~W~;>>~:>" (get symbol :type-expr)))
             (format stream " ~_= ")
             (if (boundp symbol)
               (print-value (symbol-value symbol) type stream)
               (format stream "~@<<<~;~W~;>>~:>" value-expr)))))
      (print-symbols-and-contents
       :action "Actions:" nil
       #'(lambda (action-symbol grammar-info-and-symbols stream)
           (pprint-newline :miser stream)
           (pprint-logical-block (stream (reverse grammar-info-and-symbols))
             (pprint-exit-if-list-exhausted)
             (loop
               (let* ((grammar-info-and-symbol (pprint-pop))
                      (grammar-info (car grammar-info-and-symbol))
                      (grammar (grammar-info-grammar grammar-info))
                      (grammar-symbol (cdr grammar-info-and-symbol)))
                 (write-string ": " stream)
                 (multiple-value-bind (has-type type) (action-declaration grammar grammar-symbol action-symbol)
                   (declare (ignore has-type))
                   (pprint-logical-block (stream nil)
                     (print-type type stream)
                     (format stream " ~_{~S ~S}" (grammar-info-name grammar-info) grammar-symbol))))
               (pprint-exit-if-list-exhausted)
               (pprint-newline :mandatory stream))))))))


(defmethod print-object ((world world) stream)
  (print-unreadable-object (world stream)
    (format stream "world ~A" (world-name world))))


;;; ------------------------------------------------------------------------------------------------------
;;; EVALUATION

; Scan a command.  Create types and variables in the world but do not evaluate variables' types or values yet.
; grammar-info-var is a cons cell whose car is either nil or a grammar-info for the grammar currently being defined.
(defun scan-command (world grammar-info-var command)
  (handler-bind (((or error warning)
                  #'(lambda (condition)
                      (declare (ignore condition))
                      (format *error-output* "~&~@<~2IWhile processing: ~_~:W~:>~%" command))))
    (let ((handler (and (consp command)
                        (identifier? (first command))
                        (get (world-intern world (first command)) :command))))
      (if handler
        (apply handler world grammar-info-var (rest command))
        (error "Bad command")))))


; Scan a list of commands.  See scan-command above.
(defun scan-commands (world grammar-info-var commands)
  (dolist (command commands)
    (scan-command world grammar-info-var command)))


; Compute the primitives' types from their type-exprs.
(defun define-primitives (world)
  (each-world-external-symbol-with-property
   world
   :primitive
   #'(lambda (symbol primitive)
       (declare (ignore symbol))
       (define-primitive world primitive))))


; Compute the types and values of all variables accumulated by scan-command.
(defun eval-variables (world)
  ;Compute the variables' types first.
  (each-world-external-symbol-with-property
   world
   :type-expr
   #'(lambda (symbol type-expr)
       (when (symbol-tag symbol)
         (error "~S is both a tag and a variable" symbol))
       (setf (get symbol :type) (scan-type world type-expr))))
  
  ;Then compute the variables' values.
  (let ((vars nil))
    (each-world-external-symbol-with-property
     world
     :value-expr
     #'(lambda (symbol value-expr)
         (let ((type (symbol-type symbol)))
           (if (eq (type-kind type) :->)
             (compute-variable-function symbol value-expr type)
             (push symbol vars)))))
    (mapc #'compute-variable-value vars)))


; Compute the types of all grammar declarations accumulated by scan-declare-action.
(defun eval-action-declarations (world)
  (dolist (grammar (world-grammars world))
    (each-action-declaration
     grammar
     #'(lambda (grammar-symbol action-declaration)
         (declare (ignore grammar-symbol))
         (setf (cdr action-declaration) (scan-type world (cdr action-declaration)))))))


; Compute the bodies of all grammar actions accumulated by scan-action.
(defun eval-action-definitions (world)
  (dolist (grammar (world-grammars world))
    (maphash
     #'(lambda (terminal action-bindings)
         (dolist (action-binding action-bindings)
           (unless (cdr action-binding)
             (error "Missing action ~S for terminal ~S" (car action-binding) terminal))))
     (grammar-terminal-actions grammar))
    (each-grammar-production
     grammar
     #'(lambda (production)
         (let* ((n-action-args (n-action-args grammar production))
                (codes
                 (mapcar
                  #'(lambda (action-binding)
                      (let ((action-symbol (car action-binding))
                            (action (cdr action-binding)))
                        (unless action
                          (error "Missing action ~S for production ~S" (car action-binding) (production-name production)))
                        (multiple-value-bind (has-type type) (action-declaration grammar (production-lhs production) action-symbol)
                          (declare (ignore has-type))
                          (let ((code (compute-action-code world grammar production action-symbol (action-type action) (action-expr action) type)))
                            (setf (action-code action) code)
                            (when *trace-variables*
                              (format *trace-output* "~&~@<~S[~S] := ~2I~_~:W~:>~%" action-symbol (production-name production) code))
                            code))))
                  (production-actions production)))
                (production-code
                 (if codes
                   (let* ((vars-and-rest (intern-n-vars-with-prefix "ARG" n-action-args '(stack-rest)))
                          (vars (nreverse (butlast vars-and-rest)))
                          (applied-codes (mapcar #'(lambda (code) (apply #'gen-apply code vars))
                                                 (nreverse codes))))
                     `(lambda (stack)
                        (list*-bind ,vars-and-rest stack
                          (list* ,@applied-codes stack-rest))))
                   `(lambda (stack)
                      (nthcdr ,n-action-args stack))))
                (production-code-name (unique-function-name world (string (production-name production)))))
           (setf (production-n-action-args production) n-action-args)
           (when *trace-variables*
             (format *trace-output* "~&~@<all[~S] := ~2I~_~:W~:>~%" (production-name production) production-code))
           (handler-bind (((or error warning)
                           #'(lambda (condition)
                               (declare (ignore condition))
                               (format *error-output* "~&While computing production ~S:~%" (production-name production)))))
             (quiet-compile production-code-name production-code)
             (setf (production-evaluator production) (symbol-function production-code-name))))))))


; Evaluate the given commands in the world.
; This method can only be called once.
(defun eval-commands (world commands)
  (defer-mcl-warnings
    (ensure-proper-form commands)
    (assert-true (null (world-commands-source world)))
    (setf (world-commands-source world) commands)
    (let ((grammar-info-var (list nil)))
      (scan-commands world grammar-info-var commands))
    (unite-types world)
    (eval-tags-types world)
    (define-primitives world)
    (eval-action-declarations world)
    (eval-variables world)
    (eval-action-definitions world)))


;;; ------------------------------------------------------------------------------------------------------
;;; PREPROCESSING

(defstruct (preprocessor-state (:constructor make-preprocessor-state (world)))
  (world nil :type world :read-only t)                ;The world into which preprocessed symbols are interned
  (highlight nil :type symbol)                        ;The current highlight style or nil if none
  (kind nil :type (member nil :grammar :lexer))       ;The kind of grammar being accumulated or nil if none
  (kind2 nil :type (member nil :lalr-1 :lr-1 :canonical-lr-1)) ;The kind of parser
  (name nil :type symbol)                             ;Name of the grammar being accumulated or nil if none
  (parametrization nil :type (or null grammar-parametrization)) ;Parametrization of the grammar being accumulated or nil if none
  (start-symbol nil :type symbol)                     ;Start symbol of the grammar being accumulated or nil if none
  (grammar-source-reverse nil :type list)             ;List of productions in the grammar being accumulated (in reverse order)
  (excluded-nonterminals-source nil :type list)       ;List of nonterminals to be excluded from the grammar
  (grammar-options nil :type list)                    ;List of other options for make-grammar
  (charclasses-source nil)                            ;List of charclasses in the lexical grammar being accumulated
  (lexer-actions-source nil)                          ;List of lexer actions in the lexical grammar being accumulated
  (grammar-infos-reverse nil :type list))             ;List of grammar-infos already completed (in reverse order)


; Ensure that the preprocessor-state is accumulating a grammar or a lexer.
(defun preprocess-ensure-grammar (preprocessor-state)
  (unless (preprocessor-state-kind preprocessor-state)
    (error "No active grammar at this point")))


; Finish generating the current grammar-info if one is in progress.
; Return any extra commands needed for this grammar-info.
; The result list can be mutated using nconc.
(defun preprocessor-state-finish-grammar (preprocessor-state)
  (let ((kind (preprocessor-state-kind preprocessor-state)))
    (and kind
         (let ((parametrization (preprocessor-state-parametrization preprocessor-state))
               (start-symbol (preprocessor-state-start-symbol preprocessor-state))
               (grammar-source (nreverse (preprocessor-state-grammar-source-reverse preprocessor-state)))
               (excluded-nonterminals-source (preprocessor-state-excluded-nonterminals-source preprocessor-state))
               (grammar-options (preprocessor-state-grammar-options preprocessor-state))
               (highlights (world-highlights (preprocessor-state-world preprocessor-state))))
           (multiple-value-bind (grammar lexer extra-commands)
                                (ecase kind
                                  (:grammar
                                   (values (apply #'make-and-compile-grammar
                                             (preprocessor-state-kind2 preprocessor-state)
                                             parametrization
                                             start-symbol
                                             grammar-source
                                             :excluded-nonterminals excluded-nonterminals-source
                                             :highlights highlights
                                             grammar-options)
                                           nil
                                           nil))
                                  (:lexer 
                                   (multiple-value-bind (lexer extra-commands)
                                                        (apply #'make-lexer-and-grammar
                                                          (preprocessor-state-kind2 preprocessor-state)
                                                          (preprocessor-state-charclasses-source preprocessor-state)
                                                          (preprocessor-state-lexer-actions-source preprocessor-state)
                                                          parametrization
                                                          start-symbol
                                                          grammar-source
                                                          :excluded-nonterminals excluded-nonterminals-source
                                                          :highlights highlights
                                                          grammar-options)
                                     (values (lexer-grammar lexer) lexer extra-commands))))
             (let ((grammar-info (make-grammar-info (preprocessor-state-name preprocessor-state) grammar lexer)))
               (setf (preprocessor-state-kind preprocessor-state) nil)
               (setf (preprocessor-state-kind2 preprocessor-state) nil)
               (setf (preprocessor-state-name preprocessor-state) nil)
               (setf (preprocessor-state-parametrization preprocessor-state) nil)
               (setf (preprocessor-state-start-symbol preprocessor-state) nil)
               (setf (preprocessor-state-grammar-source-reverse preprocessor-state) nil)
               (setf (preprocessor-state-excluded-nonterminals-source preprocessor-state) nil)
               (setf (preprocessor-state-grammar-options preprocessor-state) nil)
               (setf (preprocessor-state-charclasses-source preprocessor-state) nil)
               (setf (preprocessor-state-lexer-actions-source preprocessor-state) nil)
               (push grammar-info (preprocessor-state-grammar-infos-reverse preprocessor-state))
               (append extra-commands (list '(clear-grammar)))))))))


; Helper function for preprocess-source.
; source is a list of preprocessor directives and commands.  Preprocess these commands
; using the given preprocessor-state and return the resulting list of commands.
(defun preprocess-list (preprocessor-state source)
  (let ((world (preprocessor-state-world preprocessor-state)))
    (flet
      ((preprocess-one (form)
         (when (consp form)
           (let ((first (car form)))
             (when (identifier? first)
               (let ((action (symbol-preprocessor-function (world-intern world first))))
                 (when action
                   (handler-bind (((or error warning)
                                   #'(lambda (condition)
                                       (declare (ignore condition))
                                       (format *error-output* "~&~@<~2IWhile preprocessing: ~_~:W~:>~%" form))))
                     (multiple-value-bind (preprocessed-form re-preprocess) (apply action preprocessor-state form)
                       (return-from preprocess-one
                         (if re-preprocess
                           (preprocess-list preprocessor-state preprocessed-form)
                           preprocessed-form)))))))))
         (list form)))
      
      (mapcan #'preprocess-one source))))


; source is a list of preprocessor directives and commands.  Preprocess these commands
; and return the following results:
;   a list of preprocessed commands;
;   a list of grammar-infos extracted from preprocessor directives.
(defun preprocess-source (world source)
  (let* ((preprocessor-state (make-preprocessor-state world))
         (commands (preprocess-list preprocessor-state source))
         (commands (nconc commands (preprocessor-state-finish-grammar preprocessor-state))))
    (values commands (nreverse (preprocessor-state-grammar-infos-reverse preprocessor-state)))))


; Create a new world with the given name and preprocess and evaluate the given
; source commands in it.
; conditionals is an association list of (conditional . highlight), where conditional is a symbol
; and highlight is either:
;   a style keyword:   Use that style to highlight the contents of any (? conditional ...) commands
;   nil:               Include the contents of any (? conditional ...) commands without highlighting them
;   delete:            Don't include the contents of (? conditional ...) commands
(defun generate-world (name source &optional conditionals)
  (let ((world (init-world name conditionals)))
    (multiple-value-bind (commands grammar-infos) (preprocess-source world source)
      (dolist (grammar-info grammar-infos)
        (clear-actions (grammar-info-grammar grammar-info)))
      (setf (world-grammar-infos world) grammar-infos)
      (eval-commands world commands)
      world)))


;;; ------------------------------------------------------------------------------------------------------
;;; PREPROCESSOR ACTIONS


; (? <conditional> <command> ... <command>)
;   ==>
; (%highlight <highlight> <command> ... <command>)
;   or
; <empty>
(defun preprocess-? (preprocessor-state command conditional &rest commands)
  (declare (ignore command))
  (let ((highlight (resolve-conditional (preprocessor-state-world preprocessor-state) conditional))
        (saved-highlight (preprocessor-state-highlight preprocessor-state)))
    (cond
     ((eq highlight 'delete) (values nil nil))
     ((eq highlight saved-highlight) (values commands t))
     (t (values
         (unwind-protect
           (progn
             (setf (preprocessor-state-highlight preprocessor-state) highlight)
             (list (list* '%highlight highlight (preprocess-list preprocessor-state commands))))
           (setf (preprocessor-state-highlight preprocessor-state) saved-highlight))
         nil)))))


; commands is a list of commands and/or (? <conditional> ...), where the ... is a list of commands.
; Call f on each non-deleted command, passing it that command and the current value of highlight.
; f returns a list of preprocessed commands; return the destructive concatenation of these lists.
(defun each-preprocessed-command (f preprocessor-state commands highlight)
  (mapcan
   #'(lambda (command)
       (if (and (consp command) (eq (car command) '?))
         (progn
           (assert-type command (cons t cons t (list t)))
           (let* ((commands (cddr command))
                  (new-highlight (resolve-conditional (preprocessor-state-world preprocessor-state) (second command))))
             (cond
              ((eq new-highlight 'delete))
              ((eq new-highlight highlight) (each-preprocessed-command f preprocessor-state commands new-highlight))
              (t (list (list* '? (second command) (each-preprocessed-command f preprocessor-state commands new-highlight)))))))
         (funcall f command highlight)))
   commands))


; (define <name> <type> <value>)
;   ==>
; (define <name> <type> <value>)
;
; (define (<name> (<arg1> <type1>) ... (<argn> <typen>)) <result-type> . <statements>)
;   ==>
; (defun <name> (-> (<type1> ... <typen>) <result-type>)
;    (lambda ((<arg1> <type1>) ... (<argn> <typen>)) <result-type> . <statements>))
(defun preprocess-define (preprocessor-state command name type &rest value-or-statements)
  (declare (ignore command preprocessor-state))
  (values
   (list
    (if (consp name)
      (let ((bindings (rest name)))
        (list 'defun
              (first name)
              (list '-> (mapcar #'second bindings) type)
              (list* 'lambda bindings type value-or-statements)))
      (list* 'define name type value-or-statements)))
   nil))


; (action <action-name> <production-name> <type> <n-productions> <value>)
;   ==>
; (action <action-name> <production-name> <type> <n-productions> <value>)
;
; (action (<action-name> (<arg1>) ... (<argn>)) <production-name> (-> (<type1> ... <typen>) <result-type>) <n-productions> . <statements>)
;   ==>
; (actfun <action-name> <production-name> (-> (<type1> ... <typen>) <result-type>) <n-productions>
;    (lambda ((<arg1> <type1>) ... (<argn> <typen>)) <result-type> . <statements>))
(defun preprocess-action (preprocessor-state command action-name production-name type n-productions &rest value-or-statements)
  (declare (ignore command preprocessor-state))
  (values
   (list
    (if (consp action-name)
      (let ((action-name (first action-name))
            (abbreviated-bindings (rest action-name)))
        (unless (and (consp type) (eq (first type) '->))
          (error "Destructuring requires ~S to be a -> type" type))
        (let ((->-parameters (second type))
              (->-result (third type)))
          (unless (= (length ->-parameters) (length abbreviated-bindings))
            (error "Parameter count mistmatch: ~S and ~S" ->-parameters abbreviated-bindings))
          (let ((bindings (mapcar #'(lambda (binding type)
                                      (if (consp binding)
                                        (list* (first binding) type (rest binding))
                                        (list binding type)))
                                  abbreviated-bindings
                                  ->-parameters)))
            (list 'actfun action-name production-name type n-productions (list* 'lambda bindings ->-result value-or-statements)))))
      (list* 'action action-name production-name type n-productions value-or-statements)))
   nil))


(defun preprocess-grammar-or-lexer (preprocessor-state kind kind2 name start-symbol &rest grammar-options)
  (assert-type name identifier)
  (let ((commands (preprocessor-state-finish-grammar preprocessor-state)))
    (when (find name (preprocessor-state-grammar-infos-reverse preprocessor-state) :key #'grammar-info-name)
      (error "Duplicate grammar ~S" name))
    (setf (preprocessor-state-kind preprocessor-state) kind)
    (setf (preprocessor-state-kind2 preprocessor-state) kind2)
    (setf (preprocessor-state-name preprocessor-state) name)
    (setf (preprocessor-state-parametrization preprocessor-state) (make-grammar-parametrization))
    (setf (preprocessor-state-start-symbol preprocessor-state) start-symbol)
    (setf (preprocessor-state-grammar-options preprocessor-state) grammar-options)
    (values
     (nconc commands (list (list 'set-grammar name)))
     nil)))


; (grammar <name> <kind> <start-symbol>)
;   ==>
; grammar:
;   Begin accumulating a grammar with the given name and start symbol;
; commands:
;   (set-grammar <name>)
(defun preprocess-grammar (preprocessor-state command name kind2 start-symbol)
  (declare (ignore command))
  (preprocess-grammar-or-lexer preprocessor-state :grammar kind2 name start-symbol))


(defun generate-line-break-constraints (terminal)
  (assert-type terminal user-terminal)
  (list 
   (list terminal :line-break)
   (list (make-lf-terminal terminal) :no-line-break)))


; (line-grammar <name> <kind> <start-symbol>)
;   ==>
; grammar:
;   Begin accumulating a grammar with the given name and start symbol.
;   Allow :no-line-break constraints.
; commands:
;   (set-grammar <name>)
(defun preprocess-line-grammar (preprocessor-state command name kind2 start-symbol)
  (declare (ignore command))
  (preprocess-grammar-or-lexer preprocessor-state :grammar kind2 name start-symbol
                               :variant-constraint-names '(:line-break :no-line-break)
                               :variant-generator #'generate-line-break-constraints))


; (lexer <name> <kind> <start-symbol> <charclasses-source> <lexer-actions-source>)
;   ==>
; grammar:
;   Begin accumulating a lexer with the given name, start symbol, charclasses, and lexer actions;
; commands:
;   (set-grammar <name>)
(defun preprocess-lexer (preprocessor-state command name kind2 start-symbol charclasses-source lexer-actions-source)
  (declare (ignore command))
  (multiple-value-prog1
    (preprocess-grammar-or-lexer preprocessor-state :lexer kind2 name start-symbol)
    (setf (preprocessor-state-charclasses-source preprocessor-state) charclasses-source)
    (setf (preprocessor-state-lexer-actions-source preprocessor-state) lexer-actions-source)))


; (grammar-argument <argument> <attribute> <attribute> ... <attribute>)
;   ==>
; grammar parametrization:
;   (<argument> <attribute> <attribute> ... <attribute>)
; commands:
;   (grammar-argument <argument> <attribute> <attribute> ... <attribute>)
(defun preprocess-grammar-argument (preprocessor-state command argument &rest attributes)
  (preprocess-ensure-grammar preprocessor-state)
  (grammar-parametrization-declare-argument (preprocessor-state-parametrization preprocessor-state) argument attributes)
  (values (list (list* command argument attributes))
          nil))


; (production <lhs> <rhs> <name> (<action-spec-1> <type-1> . <body-1>) ... (<action-spec-n> <type-n> . <body-n>))
;   ==>
; grammar:
;   (<lhs> <rhs> <name> <current-highlight>)
; commands:
;   (%rule <lhs>)
;   (action <action-spec-1> <name> <type-1> 1 . <body-1>)
;   ...
;   (action <action-spec-n> <name> <type-n> 1 . <body-n>)
(defun preprocess-production (preprocessor-state command lhs rhs name &rest actions)
  (declare (ignore command))
  (assert-type actions (list (cons t (cons t t))))
  (preprocess-ensure-grammar preprocessor-state)
  (push (list lhs rhs name (preprocessor-state-highlight preprocessor-state))
        (preprocessor-state-grammar-source-reverse preprocessor-state))
  (values
   (cons (list '%rule lhs)
         (mapcar #'(lambda (action)
                     (list* 'action (first action) name (second action) 1 (cddr action)))
                 actions))
   t))


; (rule <general-grammar-symbol>
;       ((<action-name-1> <type-1>) ... (<action-name-n> <type-n>))
;   (production <lhs-1> <rhs-1> <name-1> (<action-spec-1-1> . <body-1-1>) ... (<action-spec-1-n> . <body-1-n>))
;   ...
;   (production <lhs-m> <rhs-m> <name-m> (<action-spec-m-1> . <body-m-1>) ... (<action-spec-m-n> . <body-m-n>)))
;   ==>
; grammar:
;   (<lhs-1> <rhs-1> <name-1> <current-highlight>)
;   ...
;   (<lhs-m> <rhs-m> <name-m> <current-highlight>)
; commands:
;   (%rule <lhs-1>)
;   ...
;   (%rule <lhs-m>)
;   (declare-action <action-name-1> <general-grammar-symbol> <type-1> <n-productions>)
;      (action <action-spec-1-1> <name-1> <type-1> <n-productions> . <body-1-1>)
;      ...
;      (action <action-spec-m-1> <name-m> <type-1> <n-productions> . <body-m-1>)
;   ...
;   (declare-action <action-name-n> <general-grammar-symbol> <type-n> <n-productions>)
;      (action <action-spec-1-n> <name-1> <type-n> <n-productions> . <body-1-n>)
;      ...
;      (action <action-spec-m-n> <name-m> <type-n> <n-productions> . <body-m-n>)
;
; The productions may be enclosed by (? <conditional> ...) preprocessor actions.
(defun preprocess-rule (preprocessor-state command general-grammar-symbol action-declarations &rest productions)
  (declare (ignore command))
  (assert-type action-declarations (list (tuple symbol t)))
  (preprocess-ensure-grammar preprocessor-state)
  (labels
    ((actions-match (action-declarations actions)
       (or (and (endp action-declarations) (endp actions))
           (let ((declared-action-name (caar action-declarations))
                 (action-name (caar actions)))
             (when (consp action-name)
               (setq action-name (first action-name)))
             (and (eq declared-action-name action-name)
                  (actions-match (rest action-declarations) (rest actions)))))))
    
    (let* ((n-productions 0)
           (commands-reverse
            (nreverse
             (each-preprocessed-command
              #'(lambda (production highlight)
                  (assert-true (eq (first production) 'production))
                  (let ((lhs (second production))
                        (rhs (third production))
                        (name (assert-type (fourth production) symbol))
                        (actions (assert-type (cddddr production) (list (cons t t)))))
                    (unless (actions-match action-declarations actions)
                      (error "Action name mismatch: ~S vs. ~S" action-declarations actions))
                    (push (list lhs rhs name highlight) (preprocessor-state-grammar-source-reverse preprocessor-state))
                    (incf n-productions)
                    (list (list '%rule lhs))))
              preprocessor-state
              productions
              (preprocessor-state-highlight preprocessor-state)))))
      (dotimes (i (length action-declarations))
        (let ((action-declaration (nth i action-declarations)))
          (push (list 'declare-action (first action-declaration) general-grammar-symbol (second action-declaration) n-productions) commands-reverse)
          (setq commands-reverse
                (nreconc
                 (each-preprocessed-command
                  #'(lambda (production highlight)
                      (declare (ignore highlight))
                      (let ((name (fourth production))
                            (action (nth (+ i 4) production)))
                        (list (list* 'action (first action) name (second action-declaration) n-productions (rest action)))))
                  preprocessor-state
                  productions
                  (preprocessor-state-highlight preprocessor-state))
                 commands-reverse))))
      (values (nreverse commands-reverse) t))))


; (exclude <lhs> ... <lhs>)
;   ==>
; grammar excluded nonterminals:
;   <lhs> ... <lhs>;
(defun preprocess-exclude (preprocessor-state command &rest excluded-nonterminals-source)
  (declare (ignore command))
  (preprocess-ensure-grammar preprocessor-state)
  (setf (preprocessor-state-excluded-nonterminals-source preprocessor-state)
        (append excluded-nonterminals-source (preprocessor-state-excluded-nonterminals-source preprocessor-state)))
  (values nil nil))


;;; ------------------------------------------------------------------------------------------------------
;;; DEBUGGING

(defmacro fsource (name)
  `(function-lambda-expression #',name))

(defmacro =source (name)
  `(function-lambda-expression (get ',name :tag=)))


#|
(defun test ()
  (handler-bind ((ccl::undefined-function-reference
                  #'(lambda (condition)
                      (break)
                      (muffle-warning condition))))
    (let ((s1 (gentemp "TEMP"))
          (s2 (gentemp "TEMP")))
      (compile s1 `(lambda (x) (,s2 x y)))
      (compile s2 `(lambda (x) (,s1 x))))))
|#