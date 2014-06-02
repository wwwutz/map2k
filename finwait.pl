#!/usr/local/bin/perl -w
my $jobfilename='job.stdout';
my $numjobs=$ARGV[0] || 1500;
$|=1;
while(1) {
  opendir(DIR, '.') || die "can't opendir $!";
  my @FILES = grep {/^$jobfilename\.\d+$/} readdir(DIR);
  closedir DIR;
  
  my @jobs = map {/^$jobfilename\.(\S+)$/;$1} @FILES;
  my $r = fromto(@jobs);
  if (defined($r) ) {
    print "$r\n";  
    last if ($r =~ m/^0+1-$numjobs/);
  }
  sleep 10;
  
}

sub fromto {
  my @RANGE = @_;
  
  my (@i, $next, $prev, $start, $end, $nextend, $string, $sstart);

  @i = sort {
         my ($aa,$bb,$aaa,$bbb);       
         $aaa = $aa = $a;
         $bbb = $bb = $b;
         if(ref($a)) { 
           $aa  = $a->[0]; 
           $aaa = $a->[1]
         }
         if(ref($b)) { 
           $bb  = $b->[0]; 
           $bbb = $b->[1]
         }
         $aa  <=> $bb ||
         $aaa <=> $bbb
       } map { 
         my @x = split /-/; 
         (@x==2)?([sort {$a <=> $b} @x]):$_ 
       } map {
         split /,+/
       } @RANGE;

  while(defined ($_ = shift @i)) { 
    if(ref($_)) {
      $start = $_->[0];
      $end   = $_->[1];
    } else {
      $start = $end = $_;
    }
    $next = undef;

    # discard dupes in next items..!
    while(exists $i[0] and not defined $next) {
      if(ref($i[0])) {
        $next    = $i[0]->[0];
        $nextend = $i[0]->[1];
      } else {
        $nextend = $next = $i[0];
      }
      if($start <= $next and $nextend <= $end) {
           $next = undef;
           shift @i;
      }
    }
    # last element
    unless(defined $next) {
      $string .= (defined $sstart and $end-$sstart-1)?"-":"," if(defined $prev);
      $string .= $start . "-" if($start < $end and not defined $sstart);
      $string .= $end;
      last;
    }

    # sequenz end
    if(defined $sstart and $next > $end+1) {
      $string .= ($end-$sstart-1)?"-":",";
      $string .= $end;
      $prev    = $end;
      $sstart  = undef;
      next;
    } 

    # sequenz start 
    if(not defined $sstart and ( ($next <= $end+1)) ) {
      $string .= "," if(defined $prev);
      $string .= $start;
      $prev    = $end;
      $sstart  = $start;
      next;
    }

    # normal
    unless(defined $sstart) {
      $string .= "," if(defined $prev);
      $string .= $start;
      $string .= "-" . $end if($start < $end);
      $prev    = $end;
      next;
    }

    # in serie -> do nothing.. 
  }    

  return $string;

}

