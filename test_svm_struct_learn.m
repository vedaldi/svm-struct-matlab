function test_svm_struct_learn
% TEST_SVM_STRUCT_LEARN
%   A demo function for SVM_STRUCT_LEARN(). It shows how to use
%   SVM-struct to learn a standard linear SVM.

  randn('state',0) ;
  rand('state',0) ;

  % ------------------------------------------------------------------
  %                                                      Generate data
  % ------------------------------------------------------------------

  th = pi/3 ;
  c = cos(th) ;
  s = sin(th) ;

  patterns = {} ;
  labels = {} ;
  for i=1:100
    patterns{i} = diag([2 .5]) * randn(2, 1) ;
    labels{i}   = 2*(randn > 0) - 1 ;
    patterns{i}(2) = patterns{i}(2) + labels{i} ;
    patterns{i} = [c -s ; s c] * patterns{i}  ;
  end

  % ------------------------------------------------------------------
  %                                                    Run SVM struct
  % ------------------------------------------------------------------

  parm.patterns = patterns ;
  parm.labels = labels ;
  parm.lossFn = @lossCB ;
  parm.constraintFn  = @constraintCB ;
  parm.featureFn = @featureCB ;
  parm.dimension = 2 ;
  parm.verbose = 1 ;
  model = svm_struct_learn(' -c 1.0 -o 1 -v 1 ', parm) ;
  w = model.w ;

  % ------------------------------------------------------------------
  %                                                              Plots
  % ------------------------------------------------------------------

  figure(1) ; clf ; hold on ;
  x = [patterns{:}] ;
  y = [labels{:}] ;
  plot(x(1, y>0), x(2,y>0), 'g.') ;
  plot(x(1, y<0), x(2,y<0), 'r.') ;
  set(line([0 w(1)], [0 w(2)]), 'color', 'y', 'linewidth', 4) ;
  xlim([-3 3]) ;
  ylim([-3 3]) ;
  set(line(10*[w(2) -w(2)], 10*[-w(1) w(1)]), ...
      'color', 'y', 'linewidth', 2, 'linestyle', '-') ;
  axis equal ;
  set(gca, 'color', 'b') ;
  w
end

% ------------------------------------------------------------------
%                                               SVM struct callbacks
% ------------------------------------------------------------------

function delta = lossCB(param, y, ybar)
  delta = double(y ~= ybar) ;
  if param.verbose
    fprintf('delta = loss(%3d, %3d) = %f\n', y, ybar, delta) ;
  end
end

function psi = featureCB(param, x, y)
  psi = sparse(y*x/2) ;
  if param.verbose
    fprintf('w = psi([%8.3f,%8.3f], %3d) = [%8.3f, %8.3f]\n', ...
            x, y, full(psi(1)), full(psi(2))) ;
  end
end

function yhat = constraintCB(param, model, x, y)
% slack resaling: argmax_y delta(yi, y) (1 + <psi(x,y), w> - <psi(x,yi), w>)
% margin rescaling: argmax_y delta(yi, y) + <psi(x,y), w>
  if dot(y*x, model.w) > 1, yhat = y ; else yhat = - y ; end
  if param.verbose
    fprintf('yhat = violslack([%8.3f,%8.3f], [%8.3f,%8.3f], %3d) = %3d\n', ...
            model.w, x, y, yhat) ;
  end
end
