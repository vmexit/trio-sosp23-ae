TARGET="`echo $TARGET`"

gplt_ver=`gnuplot --version |awk '{print $2}'`

set macros

if( gplt_ver >= 5.0){
   #command line argument '$0~$9' has been deprecated in 5.0
  if (ARG1 eq "") {
    # Sized for one column of a two column, 7.5" wide body
    # SIZE="3.05in,1.8in"

    # Sized for one column 6" wide body
    SIZE="3in,2.2in"
  } else {
    if (ARG1 eq "2col") {
      # Sized for 6" wide body
      #SIZE="2.95in,2.2in"
      # 5.5" wide body
      SIZE="2.7in,2.2in"
    } else {
      if (ARG1 eq "3col") {
        SIZE="2.25in,1.6in"
      } else {
        if (ARG1 eq "2x2") {
          # Sized for a 2x2 multiplot on a 6" wide body
          #SIZE="6in,4in"
          # 5.5" wide body
          SIZE="5.5in,3.7in"
        } else {
          SIZE=ARG1
        }
      }
    }
  }
}else{
  if ("$0" eq "" || "$0"[0:1] eq "$$"[0:1]) {
    # Sized for one column of a two column, 7.5" wide body
    # SIZE="3.05in,1.8in"

    # Sized for one column 6" wide body
    SIZE="3in,2.2in"
  } else {
    if ("$0" eq "2col") {
      # Sized for 6" wide body
      #SIZE="2.95in,2.2in"
      # 5.5" wide body
      SIZE="2.7in,2.2in"
    } else {
      if ("$0" eq "3col") {
        SIZE="2.25in,1.6in"
      } else {
        if ("$0" eq "2x2") {
          # Sized for a 2x2 multiplot on a 6" wide body
          #SIZE="6in,4in"
          # 5.5" wide body
          SIZE="5.5in,3.7in"
        } else {
          SIZE="$0"
        }
      }
    }
  }
}
if (!exists("SLIDES_SIZE")) {
  SLIDES_SIZE="720,500"
}

# Note: If you change the default font size, change \gpcode
TIKZ_FONT=exists("TIKZ_FONT") ? TIKZ_FONT : "'\\figureversion{tab},10'"
if (TARGET eq "paper-tikz") {
  set term tikz size @SIZE font @TIKZ_FONT
  set output
  set pointsize 1.5
  set key spacing 1.35
} else {
  if (TARGET eq "pdf") {
    set term pdfcairo size @SIZE linewidth 2 rounded font ',10'
    set output
  } else {
    if (TARGET eq "slides") {
      set term svg size @SLIDES_SIZE font "Open Sans,20" dashed linewidth 2 enhanced
#      set output
      set output "|sed 's/<svg/& style=\"font-weight:300\"/'"
    } else {
      if (!(TARGET eq "")) {
        if (TARGET eq "paper-epslatex") {
          set term epslatex color colortext size @SIZE input font 6 header "\\scriptsize"
          set output
          set pointsize 1.5
          set key spacing 1.35
        } else {
          print sprintf("Unknown target %s!", TARGET)
        }
      }
    }
  }
}

set ytics nomirror
set xtics nomirror
set grid back lt 0 lt rgb '#999999'
set border 3 back

set linetype 1 lw 1 lc rgb '#00dd00'
set linetype 2 lw 1 lc rgb '#0000ff'
set linetype 3 lw 1 lc rgb '#ff0000'
set linetype 10 lw 1 lc rgb 'black'

# https://bit.ly/3wrgciA
set style line 1 lt rgb "#E69F00" lw 1 pt 1 #orange
set style line 2 lt rgb "#56B4E9" lw 1 pt 2 #skyblue
set style line 3 lt rgb "#009E73" lw 1 pt 5 #green
set style line 4 lt rgb "#F0E442" lw 1 pt 7 #yellow
set style line 5 lt rgb "#0072B2" lw 3 pt 13 #blue
set style line 6 lt rgb "#D55E00" lw 1 pt 9 #vermilion
set style line 7 lt rgb "#CC79A7" lw 1 pt 3 #pink

C1 = "#E69F00"
C2 = "#56B4E9"
C3 = "#009E73"
C4 = "#F0E442"
C5 = "#0072B2"
C6 = "#D55E00"
C7 = "#CC79A7"

set style line 10 lc rgb 'black' lt 1 lw 1.5

hist_pattern_0=7
hist_pattern_1=1
hist_pattern_2=2
hist_pattern_3=7

# line style
ext = 1
extr = 2
pmfs = 3
odinfs = 4
nova = 5
sufs = 6
winefs = 7

# color
cext = C1
cextr = C2
cpmfs = C3
codinfs = C4
cnova = C5
csufs = C6
cwinefs = C7

# pattern
pstock = 6
pconcord = 10
plockstat = 17
pvictim= 10

set ytics nomirror
set xtics nomirror
set grid back lt 0 lt rgb '#999999'
set border 3 back

if( gplt_ver >= 5.0){
  set style line 1 dt 1 lc rgb C1 lw 4
  set style line 2 dt (2,2) lc rgb C2 lw 4
  set style line 3 dt (1,1) lc rgb C3 lw 4
  set style line 4 dt 3 lc rgb C4 lw 4
  set style line 5 dt 4 lc rgb C5 lw 4
  set style line 6 dt 5 lc rgb C6 lw 4
  set style line 7 dt 6 lc rgb C7 lw 4
}else{
  set style line 1 lt 1 lc rgb C1 lw 4
  set style line 2 lt (2,2) lc rgb C2 lw 4
  set style line 3 lt (1,1) lc rgb C3 lw 4
  set style line 4 lt 3 lc rgb C4 lw 4
  set style line 5 lt 4 lc rgb C5 lw 4
  set style line 6 lt 5 lc rgb C6 lw 4
  set style line 7 lt 6 lc rgb C7 lw 4
}

#
# Multiplot stuff
#

mp_startx=0.090                 # Left edge of col 0 plot area
mp_starty=0.120                 # Top of row 0 plot area
mp_width=0.825                  # Total width of plot area
mp_height=0.780                 # Total height of plot area
mp_colgap=0.07                  # Gap between columns
mp_rowgap=0.15                  # Gap between rows
# The screen coordinate of the left edge of column col
mp_left(col)=mp_startx + col*((mp_width+mp_colgap)/real(mp_ncols))
# The screen coordinate of the top edge of row row
mp_top(row)=1 - (mp_starty + row*((mp_height+mp_rowgap)/real(mp_nrows)))

# Set up a multiplot with w columns and h rows
mpSetup(w,h) = sprintf('\
    mp_nplot=-1; \
    mp_ncols=%d; \
    mp_nrows=%d; \
    set multiplot', w, h)
# Start the next graph in the multiplot
mpNext = '\
    mp_nplot=mp_nplot+1; \
    set lmargin at screen mp_left(mp_nplot%mp_ncols); \
    set rmargin at screen mp_left(mp_nplot%mp_ncols+1)-mp_colgap; \
    set tmargin at screen mp_top(mp_nplot/mp_ncols); \
    set bmargin at screen mp_top(mp_nplot/mp_ncols+1)+mp_rowgap; \
    unset label 1'

# Set Y axis row label such that it aligns regardless of tic width
mpRowLabel(lbl) = \
    sprintf('set label 1 "%s" at graph -0.25,0.5 center rotate',lbl)

#
# Slides stuff
#

if (TARGET eq "slides") {
  set style line 1 lt 1 lc rgb "#8ae234" lw 4
  set style line 2 lt 1 lc rgb "#000000" lw 4

  # Based on
  # http://youinfinitesnake.blogspot.com/2011/02/attractive-scientific-plots-with.html

  # Line style for axes
  #set style line 80 lt 1
  #set style line 80 lt rgb "#808080"

  # Line style for grid
  #set style line 81 lt 3  # Dotted
  #set style line 81 lt rgb "#808080" lw 0.5

  #set grid back linestyle 81
  #set border 3 back linestyle 80
}
