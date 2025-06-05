program Example;

var
  inputValue: Integer;
  outputValue: Integer;

begin
  Write('Enter a number: ');
  ReadLn(inputValue);
  outputValue := inputValue * 2;
  WriteLn('Output: ', outputValue);
end.