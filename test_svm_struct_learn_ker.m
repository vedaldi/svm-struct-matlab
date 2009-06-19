function test_svm_struct_learn_ker
% TEST_SVM_STRUCT_LEARN
%   Test function for SVM_STRUCT_LEARN().

  randn('state',0) ;
  rand('state',0) ;
  
  % ------------------------------------------------------------------
  %                                                      Generate data
  % ------------------------------------------------------------------
  
  th = pi/3 ; %rand * 2*pi ;
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

  sparm.patterns                 = patterns ;
  sparm.labels                   = labels ;
  sparm.lossFn                   = @lossCB
  sparm.findMostViolatedSlackFn  = @constraintCB ;
  sparm.kernelFn                 = @kernelCB ;


  model = svm_struct_learn(' -c 1.0 -o 1 -v 1 -t 4 ', sparm) ;  
  w = cat(2, model.svPatterns{:}) * (model.alpha .* cat(1, model.svLabels{:})) ; 

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

  % ------------------------------------------------------------------
  %                                               SVM struct callbacks
  % ------------------------------------------------------------------

  function delta = lossCB(param, y, ybar)
  % loss function delta(y, ybar)
    delta = double(y ~= ybar) ;
    fprintf('delta = loss(%3d, %3d) = %f\n', y, ybar, delta) ;
  end

  function ybar = constraintCB(param, model, x, y)
  % find slack-rescaling largest violation
  % argmax_y delta(yi, y) (1 + < psi(x,y), w > - < psi(x,yi), w >)

    if size(model.svPatterns, 2) == 0
      w = zeros(size(x)) ;
    else
      w = cat(2, model.svPatterns{:}) * (model.alpha .* cat(1, model.svLabels{:})) ; 
    end

    % return -y iif
    %  0 = delta(y, y) (1 +  <y*x,w> - <y*x,w>) 
    %    < delta(y,-y) (1 + <-y*x,w> - <y*x,w>) = 1 - 2 <y*x,w>
    if dot(y*x, w) > .5
      ybar = y ;
    else
      ybar = - y ;
    end
    fprintf('ybar = violslack([%8.3f,%8.3f], [%8.3f,%8.3f], %3d) = %3d\n', ...
            w, x, y, ybar) ;
  end

  function ybar = violMargin(param, model, w, x, y)
  % find margin-rescaling largest violation
  % argmax_y delta(yi, y) + < psi(x,y), w >

    % return y iif
    %  delta(y,y) + <y*x,w>  > delta(y,-y) + <-y*x,w>
    if dot(y*x, w) > .5
      ybar = y ;
    else
      ybar = - y ;
    end
    fprintf('ybar = violmargin([%8.3f,%8.3f], [%8.3f,%8.3f], %3d) = %3d\n', ...
            w, x, y, ybar) ;
  end

  function k = kernelCB(param, x,y, xp, yp)
    k = x' * xp * y * yp ;
  end

end
